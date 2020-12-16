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

private:
  bool read_master_settings = false;

  virtual void read_nonmodule_topic(const char *modulename, 
                                    const char *category, 
                                    const char *topic, 
                                    const char *data, 
                                    uint16_t    len, 
                                    uint8_t     transfer_ix)
  {
printf("Starting on read_nonmodule_topic '%s'\n", topic);
if (strncmp(topic, "moduleinterface/master_m1/setting/modules", 33) == 0) printf("------> Modules: '%s'\n", data);
printf("%d Category: %s\n", read_master_settings, category);
    if (!read_master_settings) return;
    bool for_me = false;
    {
      String my_name = "master_"; my_name += interfaces.get_prefix();
      for_me = strcmp(modulename, my_name.c_str())==0;
    }
    if (for_me && strcmp(category, "setting")==0) {
      // A setting for this master
      #ifdef MIMQTT_USE_JSON
      read_master_json_settings_from_buffer(interfaces, data, len);
      #else
      if (strstr(topic, "/modules")>0) {
       if (((PJONModuleInterfaceSet&)interfaces).set_interface_list(data)) {
         // The module list changed. Reconnect to get all settings from MQTT to new module objects.
printf("--> GOT NEW MODULE LIST. Reconnecting to get settings.\n");
          #ifdef DEBUG_PRINT
         DPRINTLN("--> GOT NEW MODULE LIST. Reconnecting to get settings.");
          #endif
         stop();
         start();
       }
        #ifdef DEBUG_PRINT
        DPRINT("Modules: '"); DPRINT(data); DPRINTLN("'");
        #endif
      } else if (strstr(topic, "/devid")>0) {
        // Set PJON id for master
        uint8_t device_id = (uint8_t) atoi(data);
        if (device_id != 0) ((PJONModuleInterfaceSet&)interfaces).get_link()->set_id(device_id);
      } else if (strstr(topic, "/intsettings")>0) {
        // Set time interval between exchanges
        uint32_t interval = atoi(data);
        if (interval > 100) ((PJONModuleInterfaceSet&)interfaces).set_transfer_interval(interval);
      }
      #endif
    }
printf("Finished with read_nonmodule_topic");
  }
};
