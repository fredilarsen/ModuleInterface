#pragma once

// This is for integration with an MQTT broker, giving the possibility to:
//
// 1. Expose all outputs from modules via MQTT so that systems like Home Assistant can use
//    the measurements and states as input, or to use Home Assistant to execute actions like
//    sending notifications and turning on/off smart switches not directly attached to the 
//    ModuleInterface setup.
// 2. Get values from a topic "moduleinterface/external/input" and expose them as if they were 
//    outputs from a module, so that modules can subscribe to them by name. This allows input 
//    like power prices to be collected by and forwarded by systems like Home Assistant, or to
//    let Home Assistant control or request ModuleInterfaces actions.
// 3. Expose settings for modules via MQTT. A setting change in a module or in web pages 
//    will be synchronized to the corresponding MQTT topic.
// 4. Get changed settings from MQTT topics. Any change in a setting will be synchronized 
//    to the corresponding setting in any module and any other MITransferBase (including 
//    the web server/database). This allows ModuleInterface setups to be configured via
//    MQTT in addition to / instead of the web pages.

#include <MI/ModuleInterface.h>
#include <MI/MITransferBase.h>
#include <utils/MITime.h>
#include <utils/MIUptime.h>
#include <utils/MIUtilities.h>
#include <utils/BinaryBuffer.h>

#include <ReconnectingMqttClient.h>
#ifdef MIMQTT_USE_JSON
#include <ArduinoJson.h>
#endif

// See if we have to minimize memory usage by splitting operations into smaller parts
#if defined(ARDUINO) && !defined(PJON_ESP)
#define MI_SMALLMEM
#endif

class MIMqttTransfer : public MITransferBase {
protected:
  // Configuration
  uint8_t broker_ip[4];
  uint16_t broker_port = 1883;

  // State
  ReconnectingMqttClient client;

  // Debug print related
  #if defined(DEBUG_PRINT) || defined(DEBUG_PRINT_SETTINGUPDATE_MQTT) || defined(DEBUG_PRINT_TIMES)
  uint32_t last_settings_debug_print_ms = 0;
  bool some_settings_missing = true;
  #endif


  void debug_print_missing_settings() {
    #if defined(DEBUG_PRINT) || defined(DEBUG_PRINT_SETTINGUPDATE_MQTT) || defined(DEBUG_PRINT_TIMES)
    if (some_settings_missing && mi_interval_elapsed(last_settings_debug_print_ms, 10000)) {
      some_settings_missing = false;
      for (uint8_t module_ix = 0; module_ix < interfaces.get_module_count(); module_ix++) {
        uint8_t initialized = interfaces[module_ix]->settings.get_initialized_count(),
          total = interfaces[module_ix]->settings.get_num_variables();
        if (initialized < total) {
          some_settings_missing = true;
          printf("Still missing %d settings for %s: ", (total - initialized), interfaces[module_ix]->module_name);
          for (uint8_t v = 0; v < total; v++) {
            if (!interfaces[module_ix]->settings.get_module_variable(v).is_initialized()) {
              printf("%s ", interfaces[module_ix]->settings.get_module_variable(v).name);
            }
          }
          printf("\n");
        }
      }
    }
    #endif
  }

public:
  MIMqttTransfer(ModuleInterfaceSet &module_interface_set, const uint8_t *broker_address, const uint16_t broker_port = 1883) :
    MITransferBase(module_interface_set) {
    if (broker_address) set_broker_address(broker_ip, broker_port);
    client.set_receive_callback(static_read_callback, this);
  }
  ~MIMqttTransfer() { client.stop(); }

  void set_broker_address(const uint8_t *address, uint16_t port) { 
    memcpy(broker_ip, address, 4); 
    broker_port = port;
    client.set_address(broker_ip, broker_port, "modulemaster"); 
  }

  virtual void start() {
    #ifdef MIMQTT_USE_JSON
    client.subscribe("moduleinterface/+/setting,moduleinterface/+/input", 1);
    #else
    client.subscribe("moduleinterface/+/setting/+,moduleinterface/+/input/+", 1);
    #endif
    client.start();
  }

  void stop() { client.stop(); }

  void update() { 
    put_events();
    client.update(); 
  }

  void get_settings() { client.update(); }

  void put_settings() {
    // If any setting has been modified in module, send it to the broker
    uint32_t start = millis();
    publish_to_mqtt(interfaces, client, true, transfer_ix);
    last_scan_times.last_set_settings_usage_ms = (uint32_t)(millis() - start);
  }

  void get_values() { client.update(); }

  // Publish for all modules
  void put_values() {
    // Send values (outputs) to the broker
    uint32_t start = millis();
    publish_to_mqtt(interfaces, client, false, transfer_ix);
    last_scan_times.last_set_values_usage_ms = (uint32_t)(millis() - start);
  }

  static void static_read_callback(const char *topic, const uint8_t *payload, uint16_t len, void *custom_ptr) {
    if (custom_ptr) ((MIMqttTransfer*)custom_ptr)->read_callback(topic, (const char*) payload, len);
  }

  void read_callback(const char *topic, const char *payload, uint16_t len) {
    read_from_mqtt(interfaces, topic, payload, len, transfer_ix);       
  }

  // Publish changed settings for only one module
  void put_settings(ModuleInterface &mi, bool events_only) {
    // Send values (outputs) to the broker
    BinaryBuffer buf(MI_MAX_JSON_SIZE), namebuf(1 + mi_max(MAX_MODULE_NAME_LENGTH, MVAR_COMPOSITE_NAME_LENGTH));
    publish_to_mqtt(mi, client, true, buf, namebuf, transfer_ix, events_only);
  }

  // Publish outputs for only one module
  void put_values(ModuleInterface &mi, bool events_only) {
    // Send values (outputs) to the broker
    BinaryBuffer buf(MI_MAX_JSON_SIZE), namebuf(1 + mi_max(MAX_MODULE_NAME_LENGTH, MVAR_COMPOSITE_NAME_LENGTH));
    publish_to_mqtt(mi, client, false, buf, namebuf, transfer_ix, events_only);
  }

  void put_events() {
    // Send events to MQTT
    for (uint8_t m = 0; m < interfaces.get_module_count(); m++) {
      if (interfaces[m]->outputs.has_events()) put_values(*interfaces[m], true);
      if (interfaces[m]->settings.has_events()) put_settings(*interfaces[m], true);
    }
  }

  static void publish_to_mqtt(ModuleInterfaceSet &interfaces, ReconnectingMqttClient &client, bool settings, uint8_t transfer_ix) {
    BinaryBuffer buf(MI_MAX_JSON_SIZE), namebuf(1 + mi_max(MAX_MODULE_NAME_LENGTH, MVAR_COMPOSITE_NAME_LENGTH));
    for (uint8_t m = 0; m < interfaces.get_module_count(); m++) {
      publish_to_mqtt(*interfaces[m], client, settings, buf, namebuf, transfer_ix, false);
    }
  }

  static void publish_to_mqtt(ModuleInterface &mi, ReconnectingMqttClient &client, bool settings, 
                              BinaryBuffer &buf, BinaryBuffer &namebuf, uint8_t transfer_ix, bool events_only) {
    // Scan for changes and events
    ModuleVariableSet &mvs = settings ? mi.settings : mi.outputs;
    bool some_events = false, some_changes = false;
    for (uint8_t i = 0; i < mvs.get_num_variables(); i++) {
      const ModuleVariable &v = mvs.get_module_variable(i);
      if (v.is_event()) some_events = true;
      if (v.is_changed()) some_changes = true;
    }
    if (events_only && !some_events) return;
    if (!some_changes) return;

    #ifdef MIMQTT_USE_JSON
    // Build JSON text
    DynamicJsonDocument root(MI_MAX_JSON_SIZE * 2);
    root["Name"] = mi.module_name;
    root["Prefix"] = mi.module_prefix;
    //if (some_events) root["Event"] = true; // Avoid this, it causes a temporary fast feedback loop
    JsonObject o = root.createNestedObject("Values");
    for (uint8_t i = 0; i < mvs.get_num_variables(); i++) {
      const ModuleVariable &v = mvs.get_module_variable(i);
      #ifdef MASTER_MULTI_TRANSFER
      if (v.is_initialized())
      #endif
      mv_to_json(v, o, v.name);
    }
    serializeJson(root, (char *)buf.get(), buf.length());
    #endif

    // Build topic
    String topic = "moduleinterface/";
    strncpy(namebuf.chars(), mi.module_name, namebuf.length());
    mi_lowercase(namebuf.chars());
    topic += namebuf.chars();
    topic += (settings ? "/setting" : "/output");

    #ifdef MIMQTT_USE_JSON
    // Publish JSON packet to broker
    client.publish(topic.c_str(), buf.chars(), true, 1);
    #else 
    // Publish each variable by itself
    String t, name;
    for (uint8_t i = 0; i < mvs.get_num_variables(); i++) {
      ModuleVariable &v = mvs.get_module_variable(i);
      if (is_mv_changed(v, transfer_ix) && (!events_only || v.is_event())) {
        strncpy(namebuf.chars(), v.name, namebuf.length());
        mi_lowercase(namebuf.chars());
        t = topic; t += "/"; t += namebuf.chars();
        v.get_value_as_text(buf.chars(), (uint8_t)buf.length());
        client.publish(t.c_str(), buf.chars(), true, 0);
        #if defined(MASTER_MULTI_TRANSFER) && defined(DEBUG_PRINT_SETTINGSYNC)
        if (settings) 
          printf("TO MQTT topic %s: %s bits:%d changed:%d\n", t.c_str(), buf.chars(), v.change_bits, v.is_changed());
        #endif
        // Do not transfer output again unless changed
        if (!settings) clear_mv_changed(v, transfer_ix);
      }
    }
    #endif
  }

 // Override this in derived classes to read master settings
  virtual void read_master_topic(const char *modulename, const char *category, 
                                 const char *topic, const char *data, uint16_t len, uint8_t transfer_ix) {  }

  // Override this in derived classes to read other topics
  virtual void read_nonmodule_topic(const char *modulename, const char *category, 
                                    const char *topic, const char *data, uint16_t len, uint8_t transfer_ix) {  }

  void read_from_mqtt(ModuleInterfaceSet &interfaces, const char *topic, const char *data, uint16_t len, uint8_t transfer_ix) {
    // Parse the topic
    if (!topic || !data || len==0 || strncmp(topic, "moduleinterface/", 16)!=0) return;
    const char *p = &topic[16]; // henhouse/input, henhouse/setting or similar
    const char *p2 = strchr(p, '/');
    if (!p2) return;
    String modulename = p; modulename[p2 - p] = 0; // "henhouse"
    p2++;
    String category = p2; // "input" or "setting"
    const char *p3 = strchr(p2, '/');
    if (p3) category[p3 - p2] = 0; // "setting/hhtemp" -> "setting"

    // Locate the correct ModuleInterface
    uint8_t module_ix = interfaces.find_interface_by_name_ignorecase(modulename.c_str());
    ModuleInterface *mi = (module_ix == NO_MODULE ? NULL : interfaces[module_ix]);

/* TODO: Investigate this idea
    // Support a dummy ModuleInterface named "external" to receive input values from
    // external sources without the external source writing directly to the inputs
    // of specific modules.
    if (!mi && modulename == "external") {
       module_ix = interfaces.find_interface_by_name_ignorecase(modulename.c_str());
       if (module_ix == NO_MODULE) module_ix = interfaces.add_module("external", "ex");
       mi = (module_ix == NO_MODULE ? NULL : interfaces[module_ix]);
        if (mi) {
          // Find variable ix
          // Add if not present, also update dependencies
           if (variable_ix == NO_VARIABLE) {
             // Find a user (so far unconnected)
             // Find type from user (or use float for all?)
             // Add variable if the type is known
             variable_ix = mi->outputs.add(variable_name, type); // Which type?
           }
        }
    }
*/

    #ifdef DEBUG_PRINT_SETTINGUPDATE_MQTT
    printf("Got topic '%s' for module '%s' ix %d\n", topic, modulename.c_str(), module_ix);
    #endif
    if (!mi) {
      // Read settings not meant for a module, for example master settings
      if (strncmp("master_", modulename.c_str(), 7) == 0)
        read_master_topic(modulename.c_str(), category.c_str(), topic, data, len, transfer_ix);
      else
        read_nonmodule_topic(modulename.c_str(), category.c_str(), topic, data, len, transfer_ix);
      return;
    }

    // Locate the correct ModuleVariableSet
    ModuleVariableSet *mvs = NULL;
    bool settings = false;
    #ifdef MIMQTT_USE_JSON
    // For topic moduleinterface/evttest/setting_event, change "setting_event" to "setting"
    #endif
    if (strcmp(category.c_str(), "setting") == 0) { mvs = &mi->settings; settings = true; }
    else if (strcmp(category.c_str(), "input")==0) mvs = &mi->inputs;
    if (!mvs) return;

    #ifdef MIMQTT_USE_JSON
    // Parse JSON
    int capacity = JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(255);
    DynamicJsonDocument root(capacity);
    DeserializationError result = deserializeJson(root, data);
    String name = root["Name"];    if (!mi_compare_ignorecase(name.c_str(), modulename.c_str(), modulename.length())) return; // Payload meant for another module
    bool is_event = root["Event"];
    JsonObject values = root["Values"];
    for (JsonPair kv : values) {
      String key = kv.key().c_str();
      uint8_t varpos = mvs->get_variable_ix_ignorecase(key.c_str());
      if (varpos != NO_VARIABLE) {
        ModuleVariable &mv = mvs->get_module_variable(varpos), mv_new(mv);
        json_to_mv(mv_new, values, key.c_str());
        if (!settings) mv.set_changed(false); // Input only travel to modules
        set_mv_and_changed_flags(mv, mv_new.get_value_pointer(), mv_new.get_size(), transfer_ix);
        if (mv.is_changed() && is_event) mv.set_event(true); // Trigger immediate transfer to modules
        #ifdef MASTER_MULTI_TRANSFER
        mv.set_initialized();
        #endif
      }
    }
    #if defined(MASTER_MULTI_TRANSFER)
    if (mvs->is_initialized()) mvs->set_updated(); // All variables have been set, and one was just updated
    #endif
    #else // Read a single variable by itself
    // Extract variable name from topic
    if (!p3) return;
    p3++;
    String variable_name = p3;
    bool is_event = find_and_remove_suffix(variable_name, "_event");
    // Locate and set the variable
    uint8_t varpos = mvs->get_variable_ix_ignorecase(variable_name.c_str());
    if (varpos != NO_VARIABLE) {
      ModuleVariable &mv = mvs->get_module_variable(varpos), mv_new(mv);
      #if defined(MASTER_MULTI_TRANSFER) && defined(DEBUG_PRINT_SETTINGSYNC)
      bool prev_changed = mv.is_changed();
      uint8_t prev_bits = mv.change_bits;
      #endif
      // Do not allow change from the server if a change is coming from module
      mv_new.set_value_from_text(data);
      if (!settings) mv.set_changed(false); // Input only travel to modules
      set_mv_and_changed_flags(mv, mv_new.get_value_pointer(), mv_new.get_size(), transfer_ix);
      if (mv.is_changed() && is_event) mv.set_event(true); // Trigger immediate transfer to modules
      #if defined(MASTER_MULTI_TRANSFER)
      mv.set_initialized();
      if (mvs->is_initialized()) {
        #if defined(DEBUG_PRINT) || defined(DEBUG_PRINT_SETTINGUPDATE_MQTT) || defined(DEBUG_PRINT_TIMES)
        if (settings && !mvs->is_updated())
          printf("GOT ALL settings for %s\n", modulename.c_str());
        #endif
        mvs->set_updated(); // All variables have been set, and one was just updated
      }
      #ifdef DEBUG_PRINT_SETTINGSYNC
      if (prev_bits != mv.change_bits) 
        printf("FROM MQTT '%s' ix: %d VALUE %ld cbits: %d->%d changed:%d->%d->%d\n", variable_name.c_str(), varpos, 
          mv.get_uint32(), prev_bits, mv.change_bits, prev_changed, mv_new.is_changed(), mv.is_changed());
      #endif
      #endif
    }
    #endif
  }

  static bool find_and_remove_suffix(String &s, const char *suffix) {
    uint8_t l = (uint8_t) strlen(suffix), l2 = (uint8_t) s.length();
    if (l2 > l && strcmp(&s[l2 - l], suffix) == 0) {
      s[l2 - l] = 0; // Remove suffix
      return true;
    }
    return false;
  }
};

