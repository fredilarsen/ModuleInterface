#pragma once

// This is for integration with an MQTT broker, giving the possibility to:
//
// 1. Expose all outputs from modules via MQTT so that systems like Home Assistant can use
//    the measurements and states as input, or to use Home Assistant to execute actions like
//    sending notifications and turning on/off smart switches not directly attached to the 
//    ModuleInterface setup.
// 2. Get inputs from MQTT topics, to allow input like power prices to be collected by
//    and forwarded by systems like Home Assistant, or to let Home Assistant control or request 
//    ModuleInterfaces actions. Inputs starting with mq, and inputs not available from
//    any other module will be used if available in the MQTT broker input topic.
// 3. Get settings from MQTT topics. Any setting written as a topic will override 
//    the corresponding setting in any module.

#include <MI/ModuleInterface.h>
#include <MI/MITransferBase.h>
#include <utils/MITime.h>
#include <utils/MIUptime.h>
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

  void start() {
    #ifdef MIMQTT_USE_JSON
    client.subscribe("moduleinterface/+/setting,moduleinterface/+/input", 1);
    #else
    client.subscribe("moduleinterface/+/setting/+,moduleinterface/+/input/+", 1);
    #endif
    client.start();
  }

  void stop() { client.stop(); }

  void update() { client.update(); }

  void get_settings() { update(); }

  void put_settings() {
    // If any setting has been modified in module, send it to the broker
    uint32_t start = millis();
    publish_to_mqtt(interfaces, client, true, transfer_ix);
    last_scan_times.last_set_settings_usage_ms = (uint32_t)(millis() - start);
  }

  void get_values() { update(); }

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
  void put_settings(ModuleInterface &mi) {
    // Send values (outputs) to the broker
    BinaryBuffer buf(MI_MAX_JSON_SIZE), namebuf(1 + mi_max(MAX_MODULE_NAME_LENGTH, MVAR_COMPOSITE_NAME_LENGTH));
    publish_to_mqtt(mi, client, true, buf, namebuf, transfer_ix);
  }

  // Publish outputs for only one module
  void put_values(ModuleInterface &mi) {
    // Send values (outputs) to the broker
    BinaryBuffer buf(MI_MAX_JSON_SIZE), namebuf(1 + mi_max(MAX_MODULE_NAME_LENGTH, MVAR_COMPOSITE_NAME_LENGTH));
    publish_to_mqtt(mi, client, false, buf, namebuf, transfer_ix);
  }

  static void publish_to_mqtt(ModuleInterfaceSet &interfaces, ReconnectingMqttClient &client, bool settings, uint8_t transfer_ix) {
    BinaryBuffer buf(MI_MAX_JSON_SIZE), namebuf(1 + mi_max(MAX_MODULE_NAME_LENGTH, MVAR_COMPOSITE_NAME_LENGTH));
    for (uint8_t m = 0; m < interfaces.get_module_count(); m++) {
      publish_to_mqtt(*interfaces[m], client, settings, buf, namebuf, transfer_ix);
    }
  }

  static void publish_to_mqtt(ModuleInterface &mi, ReconnectingMqttClient &client, bool settings, 
                              BinaryBuffer &buf, BinaryBuffer &namebuf, uint8_t transfer_ix) {
    #ifdef MIMQTT_USE_JSON
    DynamicJsonDocument root(MI_MAX_JSON_SIZE * 2);
    #endif
    ModuleVariableSet &mvs = settings ? mi.settings : mi.outputs;
    #ifdef MIMQTT_USE_JSON
    // Build JSON text
    root["ContentType"] = "ModuleInterface";
    root["ModuleName"] = mi.module_name;
    root["ModulePrefix"] = mi.module_prefix;
    JsonObject o = root["Values"];
    for (uint8_t i = 0; i < mvs.get_num_variables(); i++) {
      const ModuleVariable &v = mvs.get_module_variable(i);
      mv_to_json(v, o, v.name);
    }
    serializeJson(root, (char *)buf.get(), buf.length());
    #endif
    // Publish JSON packet to broker
    String topic = "moduleinterface/";
    strncpy(namebuf.chars(), mi.module_name, namebuf.length());
    mi_lowercase(namebuf.chars());
    topic += namebuf.chars();
    topic += (settings ? "/setting" : "/output");
    #ifdef MIMQTT_USE_JSON
    client.publish(topic.c_str(), buf.chars(), true, 0);
printf("Posted to MQTT topic %s: %s\n", topic.c_str(), buf.chars());
    #else // Publish each variable by itself
    String t, name;
    for (uint8_t i = 0; i < mvs.get_num_variables(); i++) {
      ModuleVariable &v = mvs.get_module_variable(i);
      if (is_mv_changed(v, transfer_ix)) {
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

  static void read_from_mqtt(ModuleInterfaceSet &interfaces, const char *topic, const char *data, uint16_t len, uint8_t transfer_ix) {
    // Parse the topic
    if (!topic || !data || len==0 || strncmp(topic, "moduleinterface/", 16)!=0) return;
    const char *p = &topic[16]; // henhouse/inputs, henhouse/settings or similar
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
    if (!mi) return;

    // Locate the correct ModuleVariableSet
    ModuleVariableSet *mvs = NULL;
    if (strcmp(category.c_str(), "setting")==0) mvs = &mi->settings;
    else if (strcmp(category.c_str(), "input")==0) mvs = &mi->inputs;
    if (!mvs) return;

    #ifdef MIMQTT_USE_JSON
    // Parse JSON
    DynamicJsonDocument root(MI_MAX_JSON_SIZE * 2);
    DeserializationError result = deserializeJson(root, data);
    String contenttype = root["ContentType"];
    if (contenttype == "ModuleInterface") {
      String name = root["ModuleName"];
      if (name != modulename) return; // Payload meant for another module
printf("MQTT to %s\n", name.c_str());
      JsonObject values = root["Values"];
      for (auto kv : values) {
        //String key = kv.key;
        // TODO: Remember topic will be lowercase, do case insensitive comparison
        uint8_t varpos = mvs->get_variable_ix(kv.key().c_str());
        if (varpos != NO_VARIABLE) {
          json_to_mv(mvs->get_module_variable(varpos), kv.value(), kv.key().c_str());
          //            interfaces.interfaces[mod]->inputs.get_module_variable(varpos).set_value(atoi(kv.value().as<char*>()));
        }
      }
    }
    #else // Read a single variable by itself
    // Extract variable name from topic
    if (!p3) return;
    p3++;
    String variable_name = p3;
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
      set_mv_and_changed_flags(mv, mv_new.get_value_pointer(), mv_new.get_size(), transfer_ix);
      #if defined(MASTER_MULTI_TRANSFER) && defined(DEBUG_PRINT_SETTINGSYNC)
      if (prev_bits != mv.change_bits) 
        printf("FROM MQTT '%s' ix: %d VALUE %ld cbits: %d->%d changed:%d->%d->%d\n", variable_name.c_str(), varpos, 
          mv.get_uint32(), prev_bits, mv.change_bits, prev_changed, mv_new.is_changed(), mv.is_changed());
      #endif
    }
    #endif
  }
};

