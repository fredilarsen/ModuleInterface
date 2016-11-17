#pragma once

#include <ModuleInterface.h>
#include <Ethernet.h>
#include <ArduinoJson.h>


//TODO: Transfer for one module at a time to reduce the buffer size?
//Even on a Mega it will be tight otherwise.

#define NUM_SCAN_INTERVALS 4

struct MILastScanTimes {
  time_t times[NUM_SCAN_INTERVALS];
  MILastScanTimes() {
    memset(times, NUM_SCAN_INTERVALS*sizeof(time_t), 0);
  }
};

void write_http_settings_request(EthernetClient &client) {
  client.println(F("GET /get_settings.php HTTP/1.0"));
  client.println("");
}

bool json_to_mv(ModuleVariable &v, const JsonObject& root, const char *name) {
  switch(v.type) {
    case mvtBoolean: v.set_value((bool) root[name]); return true;
    case mvtUint8: v.set_value((uint8_t) root[name]); return true;
    case mvtInt8: v.set_value((int8_t) root[name]); return true;
    case mvtUint16: v.set_value((uint16_t) root[name]); return true;
    case mvtInt16: v.set_value((int16_t) root[name]); return true;
    case mvtUint32: v.set_value((uint32_t) root[name]); return true;
    case mvtInt32: v.set_value((int32_t) root[name]); return true;
    case mvtFloat32: v.set_value((float) root[name]); return true;
  }
  return false;
}

bool mv_to_json(const ModuleVariable &v, JsonObject& root, const char *name) {
  const float SYS_ZERO = -999.25; // Marker for missing value used in some proprietary systems
  switch(v.type) {
    case mvtBoolean: root[name] = v.get_bool();return true;
    case mvtUint8: root[name] = v.get_uint8(); return true;
    case mvtInt8: root[name] = v.get_int8(); return true;
    case mvtUint16: root[name] = v.get_uint16(); return true;
    case mvtInt16: root[name] = v.get_int16(); return true;
    case mvtUint32: root[name] = v.get_uint32(); return true;
    case mvtInt32: root[name] = v.get_int32(); return true;
    case mvtFloat32:
      if (v.get_float() != SYS_ZERO && !isnan(v.get_float())) {
        root[name] = v.get_float(); return true; 
      } else return false;
  }
  return false;
}

void set_time_from_json(ModuleInterfaceSet &interfaces, JsonObject& root, uint32_t delay_ms) {
 // Set time if received (as early as possible)
  uint32_t utc = (uint32_t)root["UTC"];
  if (utc != 0) {
    if (abs((int32_t)(utc - now()) > 2)) {
    #ifdef _Time_h
      #ifdef DEBUG_PRINT
        Serial.print("--> Adjusted time from web by s: "); Serial.println((int32_t) (utc - now() + delay_ms/1000ul));
      #endif
      setTime(utc + delay_ms/1000ul); // Set global system time if using the Time library
    #endif
    }
  }
  #ifdef DEBUG_PRINT
  else Serial.println("Got no time from web server!");
  #endif
}

void decode_json_settings(ModuleInterfaceSet &interfaces, JsonObject& root) { 
  // Old comment:
  // The sequence of parameters here IS important, it must follow the order in the JSON input.
  // (Which by the way is alphabetically sorted when coming from a MariaDb table.)
  // Demand that modules, and variable names within each module, are alphabetically sorted,
  // or do global sorting of variable names here? Or have a prepared index in ModuleInterfaceSet?
  // Or local index in ModuleInterface if transferring one interface at a time?  
//TODO: Check if alphabetical is a requirement -- it does not seem so any more?

  for (int i=0; i < interfaces.num_interfaces; i++) {
    if (!interfaces[i]->settings.got_contract()) continue;
    for (int j=0; j<interfaces[i]->settings.get_num_variables(); j++) {
      json_to_mv(interfaces[i]->settings.get_module_variable(j), root, interfaces[i]->settings.get_name(j));
    }
    interfaces[i]->settings.set_updated(); // Flag that settings are ready to be used
  }
}

bool read_json_settings(ModuleInterfaceSet &interfaces, EthernetClient &client, const uint8_t port = 80,
                        const uint16_t buffer_size = 2000, const uint16_t timeout_ms = 3000) {
  char *buf = new char[buffer_size];
  if (buf == NULL) {
    #ifdef DEBUG_PRINT
    Serial.println("read_json_settings OUT OF MEMORY");
    #endif
    return false; // Out of memory at the moment
  }
  int pos = 0;
  unsigned long start = millis();
  while (client.connected() && millis()-start < timeout_ms) {
    while (client.connected() && client.available() && pos < buffer_size -1) { buf[pos] = client.read(); pos++; }
  }
  buf[pos] = 0; // null-terminate
  client.stop();  // Finished using the socket, close it as soon as possible
  #ifdef DEBUG_PRINT
  Serial.print("Read "); Serial.print(pos); Serial.println(" bytes (settings) from web server.");
  #endif
  bool status = false;
  if (pos > 0) {
    uint32_t before_parsing = millis();
    // Locate JSON part of reply
    const char *jsonDecl = "Content-Type: application/json";
    char *jsonStart = strstr(buf, jsonDecl);
    if (jsonStart) {
      jsonStart += strlen(jsonDecl) + 1;
      DynamicJsonBuffer jsonBuffer;
      JsonObject& root = jsonBuffer.parseObject(jsonStart);      
      if (root.success()) {
        // Set system time if UTC was returned from server, exclude parsing time and half of retrieval time
        #ifdef DEBUG_PRINT
        uint32_t before_set = millis();
        #endif
        set_time_from_json(interfaces, root, ((uint32_t)(millis()-before_parsing)) + ((uint32_t)(before_parsing-start))/2ul);
        decode_json_settings(interfaces, root);
        #ifdef DEBUG_PRINT
          Serial.print("Get, parse, set in ms: "); Serial.print((uint32_t)(before_parsing-start)); Serial.print(" ");
          Serial.print((uint32_t)(before_set-before_parsing)); Serial.print(" ");
          Serial.println((uint32_t)(millis()-before_set));
        #endif        
        status = true;
      }
    }
  }
  
  // Deallocate buffer
  delete buf;
  
  return status;
}

bool get_settings_from_web_server(ModuleInterfaceSet &interfaces, EthernetClient &client, IPAddress &server, uint8_t port = 80) {
  int8_t code = client.connect(server, port);
  bool success = false;
  if (code == 1) { // 1=CONNECTED
    write_http_settings_request(client);
    success = read_json_settings(interfaces, client); 
  }
  #ifdef DEBUG_PRINT
  else { Serial.print("Connection to web server failed with code "); Serial.println(code); }
  #endif
  client.stop();  
  return success;
}

void post_json_to_server(EthernetClient &client, String &buf, bool update_instead_of_insert) {
  client.println(update_instead_of_insert ? F("POST /update_jsonstatus.php HTTP/1.0") : F("POST /set_jsonstatus.php HTTP/1.0"));
  client.println(F("Content-Type: application/json"));
  client.println(F("Connection: close"));
  client.print(F("Content-Length: ")); client.println(buf.length());
  client.println();  
  client.println(buf);
  client.println();
  delay(5); // Let web server get some time to read
}

void add_module_status(ModuleInterface *interface, JsonObject &root) {
  // Add status values
  String name = interface->get_prefix(); name += F("LastLife");
  root[name] = interface->get_last_alive_age();

  name = interface->get_prefix(); name += F("Uptime");
  root[name] = interface->get_uptime_s();

  name = interface->get_prefix(); name += F("MemErr");
  root[name] = interface->out_of_memory;
}

void add_json_values(ModuleInterface *interface, JsonObject &root) {
  if (!interface->outputs.got_contract() || !interface->outputs.is_updated()) return; // Values not available yet
  
  // Add output values
  for (int i=0; i<interface->outputs.get_num_variables(); i++)
    mv_to_json(interface->outputs.get_module_variable(i), root, interface->outputs.get_name(i));
  
  // Add status values
  add_module_status(interface, root);
}

void set_scan_columns(JsonObject &root,
                      MILastScanTimes *last_scan_times) // NULL or array of length 4                      
{
  if (last_scan_times) {
    // Do not sample at startup, init time array to now
    time_t curr = now();
    for (uint8_t i=0; i<NUM_SCAN_INTERVALS; i++) 
      if (last_scan_times->times[i]==0) last_scan_times->times[i] = curr;
    
    // Set one or more of the scan flags to enable plotting with different steps
    const uint8_t scan60s = 0, scan10m = 1, scan1h = 2, scan1d = 3;
    if (last_scan_times->times[scan60s] + 60 <= curr) {
      last_scan_times->times[scan60s] = curr;
      root["scan1m"] = 1;
      if (last_scan_times->times[scan10m] + 600 <= curr) {
        last_scan_times->times[scan10m] = curr;
        root["scan10m"] = 1;
        if (last_scan_times->times[scan1h] + 3600 <= curr) {
          last_scan_times->times[scan1h] = curr;
          root["scan1h"] = 1;
          if (last_scan_times->times[scan1d] + 24*3600 <= curr) {
            last_scan_times->times[scan1d] = curr;
            root["scan1d"] = 1;
    } } } }
  }
}

bool send_values_to_web_server(ModuleInterfaceSet &interfaces, EthernetClient &client, IPAddress &server,
                               MILastScanTimes *last_scan_times, uint8_t port = 80, 
                               bool update_instead_of_insert = false) {                                 
  // Encode all settings to a JSON string
  String buf;
  { // Separate block to release JSON buffer as soon as possible
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    for (int i=0; i<interfaces.num_interfaces; i++) add_json_values(interfaces[i], root);
    
    // Set scan columns in datebase if inserting (support for plotting with different resolutions)
    if (!update_instead_of_insert) set_scan_columns(root, last_scan_times);
    
    root.printTo(buf);
  }

  if (buf.length() <= 2) return true; // Empty buffer, nothing to send, so not a failure
  #ifdef DEBUG_PRINT
  Serial.print("Writing "); Serial.print(buf.length()); Serial.println(" bytes (outputs) to web server.");
  #endif

  // Post JSON to web server
  int8_t code = client.connect(server, port);
  if (code == 1) { // 1=CONNECTED
    post_json_to_server(client, buf, update_instead_of_insert);
  }  
  #ifdef DEBUG_PRINT
  else { Serial.print("Connection to web server failed with code "); Serial.println(code); }
  #endif
  client.stop();
  return code == 1;
}



