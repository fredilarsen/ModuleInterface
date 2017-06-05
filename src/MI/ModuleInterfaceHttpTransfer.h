#pragma once

#include <ArduinoJson.h>
#include <MI/ModuleInterface.h>
#include <utils/MITime.h>
#include <utils/MIUptime.h>
#include <utils/MemFrag.h>

#define NUM_SCAN_INTERVALS 4

struct MILastScanTimes {
  uint32_t times[NUM_SCAN_INTERVALS];
  MILastScanTimes() {
    memset(times, NUM_SCAN_INTERVALS*sizeof(uint32_t), 0);
  }
};

void write_http_settings_request(const char *module_prefix, EthernetClient &client) {
  String request = F("GET /get_settings.php?prefix=");
  request += module_prefix;
  client.println(request);
  client.println("");
  client.flush();
}

bool json_to_mv(ModuleVariable &v, const JsonObject& root, const char *name) {
  switch(v.get_type()) {
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

bool mv_to_json(const ModuleVariable &v, JsonObject& root, const char *name_in) {
  String name = name_in; // For some reason a char* will succeed but be forgotten. Probably added as a pointer in JSON enocder
  const float SYS_ZERO = -999.25; // Marker for missing value used in some proprietary systems
  switch(v.get_type()) {
    case mvtBoolean: root[name] = v.get_bool();return true;
    case mvtUint8: root[name] = v.get_uint8(); return true;
    case mvtInt8: root[name] = v.get_int8(); return true;
    case mvtUint16: root[name] = v.get_uint16(); return true;
    case mvtInt16: root[name] = v.get_int16(); return true;
    case mvtUint32: root[name] = v.get_uint32(); return true;
    case mvtInt32: root[name] = v.get_int32(); return true;
    case mvtFloat32:
      if (v.get_float() != SYS_ZERO && !isnan(v.get_float()) && !isinf(v.get_float())) {
        root[name] = v.get_float(); return true;
      } else return false;
  }
  return false;
}

void set_time_from_json(JsonObject& root, uint32_t delay_ms) {
 // Set time if received (as early as possible)
  uint32_t utc = (uint32_t)root["UTC"];
  if (utc != 0) {
    if (abs((int32_t)(utc -miGetTime()) > 2)) {
    #ifndef NO_TIME_SYNC
      #ifdef DEBUG_PRINT
        Serial.print(F("--> Adjusted time from web by s: ")); Serial.println((int32_t) (utc - miGetTime() + delay_ms/1000ul));
      #endif
      miSetTime(utc + delay_ms/1000ul); // Set system time
    #endif
    }
  }
  #ifdef DEBUG_PRINT
  else Serial.println(F("Got no time from web server!"));
  #endif
}

void decode_json_settings(ModuleInterface &interface, JsonObject& root) {
  if (!interface.settings.got_contract()) return;

  char prefixed_name[MVAR_MAX_NAME_LENGTH + MVAR_PREFIX_LENGTH + 1];
  for (int j=0; j<interface.settings.get_num_variables(); j++) {
    interface.settings.get_module_variable(j).get_prefixed_name(interface.get_prefix(), prefixed_name, sizeof prefixed_name);
    json_to_mv(interface.settings.get_module_variable(j), root, prefixed_name);
  }
  interface.settings.set_updated(); // Flag that settings are ready to be used
}

bool read_json_settings(ModuleInterface &interface, EthernetClient &client, const uint8_t port = 80,
                        const uint16_t buffer_size = 500, const uint16_t timeout_ms = 3000) {
  char *buf = new char[buffer_size];
  if (buf == NULL) {
    ModuleVariableSet::out_of_memory = true;
    #ifdef DEBUG_PRINT
    Serial.println(F("read_json_settings OUT OF MEMORY"));
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
//  Serial.print(F("Read ")); Serial.print(pos); Serial.print(F(" bytes (settings) from web server for "));
//  Serial.print(interface.module_name);
  #endif
  if (pos >= buffer_size - 1) {
    delete[] buf;
    ModuleVariableSet::out_of_memory = true;
    #ifdef DEBUG_PRINT
    Serial.println(); Serial.print(F("read_json_settings BUFFER TOO SMALL"));
    #endif
    return false;
  }
  bool status = false;
  if (pos > 0) {
    uint32_t before_parsing = millis();
    // Locate JSON part of reply
    const char *jsonDecl = "Content-Type: application/json";
    char *jsonStart = strstr(buf, jsonDecl);
    if (jsonStart) jsonStart += strlen(jsonDecl) + 1;
    else jsonStart = buf;
    {
      DynamicJsonBuffer jsonBuffer;
      JsonObject& root = jsonBuffer.parseObject(jsonStart);
      if (root.success()) {
        // Set system time if UTC was returned from server, exclude parsing time and half of retrieval time
        #ifdef DEBUG_PRINT
        uint32_t before_set = millis();
        #endif
        set_time_from_json(root, ((uint32_t)(millis()-before_parsing)) + ((uint32_t)(before_parsing-start))/2ul);
        decode_json_settings(interface, root);
        #ifdef DEBUG_PRINT
//        Serial.print(": Get, parse, set in ms: "); Serial.print((uint32_t)(before_parsing-start)); Serial.print(" ");
//        Serial.print((uint32_t)(before_set-before_parsing)); Serial.print(" ");
//        Serial.print((uint32_t)(millis()-before_set));
        #endif
        status = true;
      } else {
        #ifdef DEBUG_PRINT
        Serial.print("Failed parsing settings JSON. Out of memory?");
        #endif
      }
    }
  }
  #ifdef DEBUG_PRINT
  //Serial.println();
  #endif

  // Deallocate buffer
  delete[] buf;

  return status;
}

bool get_settings_from_web_server(ModuleInterfaceSet &interfaces, EthernetClient &client, IPAddress &server, uint8_t port = 80) {
  #ifdef DEBUG_PRINT
  uint32_t start_time = millis();
  #endif
  uint8_t cnt = 0;
  for (int i=0; i < interfaces.num_interfaces; i++) {
    int8_t code = client.connect(server, port);
    bool success = false;
    if (code == 1) { // 1=CONNECTED
      write_http_settings_request(interfaces[i]->module_prefix, client);
      success = read_json_settings(*interfaces[i], client);
    }
    #ifdef DEBUG_PRINT
    else { Serial.print("Connection to web server failed with code "); Serial.println(code); }
    #endif
    client.stop();
    if (success) cnt++;
  }
  #ifdef DEBUG_PRINT
  Serial.print(F("Reading settings took ")); Serial.print((uint32_t)(millis() - start_time)); Serial.println("ms.");
  #endif
  return cnt > 0;
}

void post_json_to_server(EthernetClient &client, String &buf) {
  client.println(F("POST /set_currentvalues.php HTTP/1.0"));
  client.println(F("Content-Type: application/json"));
  client.println(F("Connection: close"));
  client.print(F("Content-Length: ")); client.println(buf.length());
  client.println();
  client.println(buf);
  client.println();
  client.flush();
}

void add_module_status(ModuleInterface *interface, JsonObject &root) {
  // Add status values
  String name = interface->get_prefix(); name += F("LastLife");
  root[name] = interface->get_last_alive_age();

  name = interface->get_prefix(); name += F("Uptime");
  root[name] = interface->get_uptime_s();

  name = interface->get_prefix(); name += F("MemErr");
  root[name] = (uint8_t) interface->out_of_memory;

  name = interface->get_prefix(); name += F("StatBits");
  root[name] = (uint8_t) interface->get_status_bits();
}

void add_json_values(ModuleInterface *interface, JsonObject &root) {
  
  if (!interface->outputs.got_contract() || !interface->outputs.is_updated()) return; // Values not available yet

  // Add output values
  char prefixed_name[MVAR_MAX_NAME_LENGTH + MVAR_PREFIX_LENGTH + 1];
  for (int i=0; i<interface->outputs.get_num_variables(); i++) {
    interface->outputs.get_module_variable(i).get_prefixed_name(interface->get_prefix(), prefixed_name, sizeof prefixed_name);
    mv_to_json(interface->outputs.get_module_variable(i), root, prefixed_name);
  }

  // Add status values
  add_module_status(interface, root);
}

void set_scan_columns(JsonObject &root,
                      MILastScanTimes *last_scan_times) // NULL or array of length 4
{
  if (last_scan_times) {
    // Do not sample at startup, init time array to now
    uint32_t curr = miGetTime();
    for (uint8_t i=0; i<NUM_SCAN_INTERVALS; i++)
      if (last_scan_times->times[i]==0) last_scan_times->times[i] = curr;

    // Set one or more of the scan flags to enable plotting with different steps
    const uint8_t scan1m = 0, scan10m = 1, scan1h = 2, scan1d = 3;
    uint8_t set1m = 0, set10m = 0, set1h = 0, set1d = 0;
    if (last_scan_times->times[scan1m] + 60 <= curr) {
      last_scan_times->times[scan1m] = curr;
      set1m = 1;
      if (last_scan_times->times[scan10m] + 600 <= curr) {
        last_scan_times->times[scan10m] = curr;
        set10m = 1;
        if (last_scan_times->times[scan1h] + 3600 <= curr) {
          last_scan_times->times[scan1h] = curr;
          set1h = 1;
          if (last_scan_times->times[scan1d] + 24ul*3600 <= curr) {
            last_scan_times->times[scan1d] = curr;
            set1d = 1;
    } } } }
    root[F("scan1m")]  = set1m;
    root[F("scan10m")] = set10m;
    root[F("scan1h")]  = set1h;
    root[F("scan1d")]  = set1d;
  }
}

void add_master_status(ModuleInterfaceSet &interfaces, JsonObject &root) {
  // Add number of currently inactive (nonresponding) modules
  String name = interfaces.get_prefix(); name += F("InactCnt");
  root[name] = interfaces.get_inactive_module_count();

  uint16_t num_fragments = 0;
  size_t total_free = 0, largest_free = largest_free_block(num_fragments, total_free);
  #ifdef DEBUG_PRINT
  Serial.print(F("MEM Free=")); Serial.print(total_free); Serial.print(F(", #frag=")); Serial.print(num_fragments);
  Serial.print(", bigfrag="); Serial.println(largest_free);
  #endif
  
  // Add total free memory
  name = interfaces.get_prefix(); name += F("FreeTot");
  root[name] = total_free;

  // Add largest free block
  name = interfaces.get_prefix(); name += F("FreeMax");
  root[name] = largest_free;

  // Add number of fragments
  name = interfaces.get_prefix(); name += F("FragCnt");
  root[name] = (uint8_t) num_fragments;

  // Add out-of-memory status
  name = interfaces.get_prefix(); name += F("MemErr");
  root[name] = (uint8_t) ModuleVariableSet::out_of_memory;
  
  // Add uptime
  name = interfaces.get_prefix(); name += F("Uptime");
  root[name] = (uint32_t) miGetUptime();
  
  // Add system time
  name = interfaces.get_prefix(); name += F("UTC");
  root[name] = (uint32_t) miGetTime();
}

bool send_values_to_web_server(ModuleInterfaceSet &interfaces, EthernetClient &client, IPAddress &server,
                               MILastScanTimes *last_scan_times, uint8_t port = 80,
                               bool primary_master = true, // (set primary_master=false on all masters but one if more than one)
                               uint16_t json_buffer_size = 500) {
  int successCnt = 0;
  #ifdef DEBUG_PRINT
  uint32_t start_time = millis();
  #endif
  for (int i=0; i<interfaces.num_interfaces; i++) {
    // Encode all settings to a JSON string
    String buf;
    { // Separate block to release JSON buffer as soon as possible
      DynamicJsonBuffer jsonBuffer(json_buffer_size);
      JsonObject& root = jsonBuffer.createObject();
      add_json_values(interfaces[i], root);

      // Set scan columns in database if inserting (support for plotting with different resolutions)
      if (primary_master && i == 0) set_scan_columns(root, last_scan_times);

      // Add status for the master
      if (i == 0) add_master_status(interfaces, root);
      root.printTo(buf);
    }

    if (buf.length() <= 2) continue; // Empty buffer, nothing to send, so not a failure
    #ifdef DEBUG_PRINT
    //Serial.println(buf);
    //Serial.print(F("Writing ")); Serial.print(buf.length()); Serial.print(F(" bytes (outputs) to web server for "));
    //Serial.println(interfaces[i]->module_name);
    #endif

    // Post JSON to web server
    int8_t code = client.connect(server, port);
    if (code == 1) { // 1=CONNECTED
      post_json_to_server(client, buf);
    }
    #ifdef DEBUG_PRINT
    else { Serial.print(F("Connection to web server failed with code ")); Serial.println(code); }
    #endif
    client.stop();
    if (code == 1) successCnt++;
  }
  
  // Take a snapshot of the currentvalues table into the timeseries table
  if (primary_master) {
    int8_t code = client.connect(server, port);
    if (code == 1) { // 1=CONNECTED
      client.println(F("GET /store_currentvalues.php"));
    }
    #ifdef DEBUG_PRINT
    else { Serial.print(F("Connection to web server failed with code ")); Serial.println(code); }
    #endif
    client.stop();
  }
  
  #ifdef DEBUG_PRINT
  Serial.print(F("Writing to web server took ")); Serial.print((uint32_t)(millis() - start_time));
  Serial.println(F("ms."));
  #endif
  return successCnt > 0;
}
