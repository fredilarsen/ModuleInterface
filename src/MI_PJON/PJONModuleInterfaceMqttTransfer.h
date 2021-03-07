#pragma once

#include <MI/ModuleInterfaceMqttTransfer.h>

// Decode and activate master settings from a JSON text
bool read_master_json_settings_from_buffer(PJONModuleInterfaceSet &interfaces, 
                                           const char     *data, 
                                                 uint16_t len,
                                           const uint16_t buffer_size = MI_MAX_JSON_SIZE)
{
  DynamicJsonDocument root(buffer_size);
  auto error = deserializeJson(root, data);
  bool status = read_master_json_settings(interfaces, root, error);
  return status;
}

// This class inherits module transfer functionality from MIMqttpTransfer and adds support for reading master settings.
class PJONMIMqttTransfer : public MIMqttTransfer {
public:
  PJONMIMqttTransfer(PJONModuleInterfaceSet &module_interface_set,
                     bool read_master_settings_from_mqtt, // Whether to read master settings from MQTT or not
                     const uint8_t *broker_address, 
                     const uint16_t broker_port = 1883) : 
    MIMqttTransfer(module_interface_set, broker_address, broker_port) {
      read_master_settings = read_master_settings_from_mqtt;
    }

  // Whether to read master settings from MQTT or not
  void set_read_master_settings(bool read) { read_master_settings = read; }

  virtual void start() {
    // When starting based on master settings from MQTT, we are doing it in 3 phases:
    // 1. Get master settings, create ModuleInterface objects based on module list.
    // 2. Get contracts from all modules.
    // 3. Reconnect to MQTT now subscribing to all settings and inputs, so that
    //    these will be registered in objects and synced to modules.

    // If we have all master settings then start normally (phase 3)
    if (!read_master_settings || (got_settings() && got_contracts())) {
      phase = PHASE_RUNNING;
      MIMqttTransfer::start();
    }
    else {
      // Subscribe to master settings only (phase 1)
      phase = PHASE_WAIT_MQTT;
      #if defined(DEBUG_PRINT) || defined(DEBUG_PRINT_SETTINGUPDATE_MQTT)
      printf("%%%%%%%%%%%% PHASE_WAIT_MQTT starting at %dms.\n", millis());
      #endif
      String master_topic = "moduleinterface/master_"; master_topic += interfaces.get_prefix();
      #ifdef MIMQTT_USE_JSON
      master_topic += "/setting";
      #else
      master_topic += "/setting/+";
      #endif
      #if defined(DEBUG_PRINT) || defined(DEBUG_PRINT_SETTINGUPDATE_MQTT)
      printf("Subscribing to topic '%s'\n", master_topic.c_str());
      #endif
      client.subscribe(master_topic.c_str(), 1);
      client.start();
    }
  }

private:
  // Flags used in got_master_settings member
  const uint8_t SETTING_MODULE_LIST = 1, SETTING_DEVID = 2, SETTING_INTERVAL = 4, SETTING_ALL = 7;

  // Startup phases
  enum Phase { PHASE_STOPPED, PHASE_WAIT_MQTT, PHASE_WAIT_CONTRACTS, PHASE_RUNNING };

  bool read_master_settings = false;
  uint8_t got_master_settings = 0;  // Bitmasked, 
  enum Phase phase = PHASE_STOPPED;
  long contract_retrieval_start = 0;

  bool got_settings() const { return (got_master_settings & SETTING_ALL) == SETTING_ALL; }
  bool got_contracts() const { 
    return interfaces.got_all_contracts() 
      && ((interfaces.get_inactive_module_count() == 0) 
        || (uint32_t(millis() - contract_retrieval_start) > MI_INACTIVE_TIME_THRESHOLD*1000l));
  }

  virtual void update() {
    MIMqttTransfer::update();
    check_phase();
    if (read_master_settings) debug_print_missing_settings();
  }

  void check_phase() {
    // Check if we are ready to transition to a new connection phase
    switch(phase) {
      case PHASE_STOPPED: break;
      case PHASE_WAIT_MQTT: 
        if (got_settings()) {
          phase = PHASE_WAIT_CONTRACTS;
          contract_retrieval_start = millis();
          #if defined(DEBUG_PRINT) || defined(DEBUG_PRINT_SETTINGUPDATE_MQTT)
          printf("%%%%%%%%%%%% PHASE_WAIT_CONTRACTS starting at %dms. Got all master settings.\n", millis());
          #endif
        }
        break;
      case PHASE_WAIT_CONTRACTS: 
        if (got_contracts()) { 
          // Reconnect to MQTT broker wil full subscription, not only master settings
          #if defined(DEBUG_PRINT) || defined(DEBUG_PRINT_SETTINGUPDATE_MQTT)
          printf("%%%%%%%%%%%% PHASE_RUNNING starting at %dms. Got %d (%d inactive) modules.\n", millis(), interfaces.get_module_count(), interfaces.get_inactive_module_count());
          #endif
          phase = PHASE_RUNNING;
          stop(); 
          start();
        }
        break;
      case PHASE_RUNNING: break;
    }
  }

  virtual void read_master_topic(const char *modulename, 
                                 const char *category, 
                                 const char *topic, 
                                 const char *data, 
                                 uint16_t    len, 
                                 uint8_t     transfer_ix)
  {
    if (!read_master_settings) return;
    bool for_me = false;
    {
      String my_name = "master_"; my_name += interfaces.get_prefix();
      for_me = strcmp(modulename, my_name.c_str())==0;
    }
    if (for_me && strcmp(category, "setting")==0) {
      // A setting for this master
      #ifdef MIMQTT_USE_JSON
      bool ok = read_master_json_settings_from_buffer((PJONModuleInterfaceSet&) interfaces, data, len);
      got_master_settings = ok ? SETTING_ALL : 0;
      // Check if module list was changed and objects recreated
      bool all_empty = true;
      for (uint8_t m = 0; m < interfaces.get_module_count(); m++) {
        if (interfaces[m]->settings.get_num_variables() > 0 || interfaces[m]->outputs.get_num_variables() > 0) {
          all_empty = false; break;
        }
      }
      if (all_empty) phase = PHASE_WAIT_MQTT; // Trigger exchange of contracts followed by reconnect to get all settings
      #else
      if (strstr(topic, "/modules")>0) {
        if (((PJONModuleInterfaceSet&)interfaces).set_interface_list(data)) {
          // The module list changed. Reconnect to get all settings from MQTT to new module objects.
          got_master_settings |= SETTING_MODULE_LIST;
          phase = PHASE_WAIT_MQTT;
          #if defined(DEBUG_PRINT) || defined(DEBUG_PRINT_SETTINGUPDATE_MQTT)
          if (phase != PHASE_WAIT_MQTT) DPRINTLN("--> GOT NEW MODULE LIST. Changing to PHASE_WAIT_MQTT.");
          #endif
        }
        #if defined(DEBUG_PRINT) || defined(DEBUG_PRINT_SETTINGUPDATE_MQTT)
        DPRINT("Modules: '"); DPRINT(data); DPRINTLN("'");
        #endif
      } else if (strstr(topic, "/devid")>0) {
        // Set PJON id for master
        got_master_settings |= SETTING_DEVID;
        uint8_t device_id = (uint8_t) atoi(data);
        if (device_id != 0) ((PJONModuleInterfaceSet&)interfaces).get_link()->set_id(device_id);
      } else if (strstr(topic, "/intsettings")>0) {
        // Set time interval between exchanges
        got_master_settings |= SETTING_INTERVAL;
        uint32_t interval = atoi(data);
        if (interval > 100) ((PJONModuleInterfaceSet&)interfaces).set_transfer_interval(interval);
      }
      #endif
      check_phase();
    }
  }
};
