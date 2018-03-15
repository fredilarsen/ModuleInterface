#pragma once

#include <MI/ModuleInterface.h>
#include <utils/MITime.h>
#include <utils/MIUptime.h>
#include <utils/MemFrag.h>

#include <ArduinoJson.h>

#define NUM_SCAN_INTERVALS 4

// See if we have to minimize memory usage by splitting operations into smaller parts
#if defined(ARDUINO) && !defined(PJON_ESP)
#define MI_SMALLMEM
#endif

struct MILastScanTimes {
  uint32_t times[NUM_SCAN_INTERVALS];
  MILastScanTimes() {
    memset(times, 0, NUM_SCAN_INTERVALS*sizeof(uint32_t));
  }
};

void write_http_settings_request(const char *module_prefix, Client &client) {
  String request = F("GET /get_settings.php?prefix=");
  request += module_prefix;
  request += "\r\n\r\n";
  client.print(request.c_str());
  // NOTE: On ESP8266 a client.flush() seems to close the connection, so avoid it
}

template <class T> void val_to_buf(uint32_t *buf, T value) { memcpy(buf, &value, sizeof value); }

void buf_to_mvar(ModuleVariable &v, const uint32_t *value, const uint8_t size) {
  if (v.is_changed()) {
    // Discard changes from database if changes from module are present and not registered yet.
    // Reset changed-flag if value from database matches the current value, meaning that the change
    // from the module has been transported to the database and back.
    #ifdef DEBUG_PRINT
    DPRINT("Changed, equal="); DPRINT(v.is_equal(value, size)); DPRINT(", val="); DPRINTLN(*(const float*)value);   
    #endif
    if (v.is_equal(value, size)) v.set_changed(false);
  } else {
    v.set_value(value, size);
    v.set_changed(false); // Do not set changed-flag when set from database, only in the opposite direction
  }
}

// NOTE: The change management makes this function specific to settings.
bool json_to_mv(ModuleVariable &v, const JsonObject& root, const char *name) {
  uint32_t b;
  switch(v.get_type()) {
    // NOTE: boolean is transferred as 0/1 instead of true/false to enable plotting
    case mvtBoolean: { 
      val_to_buf(&b, (uint8_t) root[name]); 
      bool bv = *(uint8_t*)&b != 0;
      buf_to_mvar(v, (uint32_t*)&bv, sizeof(bool)); return true;
    }
    case mvtUint8: { val_to_buf(&b, (uint8_t) root[name]); buf_to_mvar(v, &b, sizeof(uint8_t)); return true; }
    case mvtInt8: val_to_buf(&b, (int8_t) root[name]); buf_to_mvar(v, &b, sizeof(int8_t)); return true;
    case mvtUint16: val_to_buf(&b, (uint16_t) root[name]); buf_to_mvar(v, &b, sizeof(uint16_t)); return true;
    case mvtInt16: val_to_buf(&b, (int16_t) root[name]); buf_to_mvar(v, &b, sizeof(int16_t)); return true;
    case mvtUint32: val_to_buf(&b, (uint32_t) root[name]); buf_to_mvar(v, &b, sizeof(uint32_t));return true;
    case mvtInt32: val_to_buf(&b, (int32_t) root[name]); buf_to_mvar(v, &b, sizeof(int32_t));return true;
    case mvtFloat32: val_to_buf(&b, (float) root[name]); buf_to_mvar(v, &b, sizeof(float)); return true;
    case mvtUnknown: return false;
  }
  return false;
}

bool mv_to_json(const ModuleVariable &v, JsonObject& root, const char *name_in) {
  String name = name_in; // For some reason a char* will succeed but be forgotten. Probably added as a pointer in JSON enocder
  const float SYS_ZERO = -999.25; // Marker for missing value used in some proprietary systems
  switch(v.get_type()) {
    case mvtBoolean: root[name] = (uint8_t) (v.get_bool() ? 1 : 0); return true;
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
    case mvtUnknown: return false;
  }
  return false;
}

void set_time_from_json(JsonObject& root, uint32_t delay_ms) {
 // Set time if received (as early as possible)
  uint32_t utc = (uint32_t)root["UTC"];
  if (utc != 0) {
    utc += delay_ms/1000ul;
    if (abs((int32_t)(utc -miGetTime()) > 2)) {
    #ifndef NO_TIME_SYNC
      #ifdef DEBUG_PRINT
        DPRINT(F("--> Adjusted time from web by s: ")); DPRINTLN((int32_t) (utc - miGetTime()));
      #endif
      miSetTime(utc); // Set system time
    #endif
    }
  }
  #ifdef DEBUG_PRINT
  else DPRINTLN(F("Got no time from web server!"));
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

JsonObject& read_json_settings_from_server(
    Client &client, DynamicJsonBuffer &jsonBuffer, char *buf, const uint16_t buffer_size, 
    const uint8_t /*port*/ = 80, const uint16_t timeout_ms = 3000) 
{
  if (buf == NULL) {
    ModuleVariableSet::out_of_memory = true;
    #ifdef DEBUG_PRINT
    DPRINTLN(F("read_json_settings OUT OF MEMORY"));
    #endif
    return JsonObject::invalid();
  }
  uint16_t pos = 0;
  uint32_t start = millis();
  while (client.connected() && (millis()-start) < timeout_ms && pos==0) {
    while (client.connected() && client.available()>0 && pos < buffer_size -1 && (millis()-start) < timeout_ms) { 
      uint16_t len = client.read((uint8_t*) &buf[pos], buffer_size - pos -1); 
      if (len > 0) pos += len; else break;
    }
    if (pos == 0) delay(1);
  }
  buf[pos] = 0; // null-terminate
  client.stop();  // Finished using the socket, close it as soon as possible
  char *jsonStart = buf;
  if (pos >= buffer_size - 1) {
    ModuleVariableSet::out_of_memory = true;
    #ifdef DEBUG_PRINT
    DPRINT(pos); DPRINTLN(F(" bytes, read_json_settings BUFFER TOO SMALL"));
    #endif
    return JsonObject::invalid();
  }
  if (pos > 0) {
    // Locate JSON part of reply
    const char *jsonDecl = "Content-Type: application/json";
    jsonStart = strstr(buf, jsonDecl);
    if (jsonStart) jsonStart += strlen(jsonDecl) + 1;
    else jsonStart = buf;
  }
  return jsonBuffer.parseObject(jsonStart);
}


bool read_json_settings(ModuleInterface &interface, Client &client, const uint8_t port = 80,
                        const uint16_t buffer_size = 800, const uint16_t timeout_ms = 3000) {
  char *buf = new char[buffer_size];
  DynamicJsonBuffer jsonBuffer;
  uint32_t start = millis();
  JsonObject& root = read_json_settings_from_server(client, jsonBuffer, buf, buffer_size, port, timeout_ms);
  bool status = false;
  if (root.success()) {
    // Set system time if UTC was returned from server, exclude parsing time and half of retrieval time
    #ifdef DEBUG_PRINT
    uint32_t before_set = millis();
    #endif
    set_time_from_json(root, (uint32_t)(millis()-start));
    decode_json_settings(interface, root);
    status = true;
  } else {
    #ifdef DEBUG_PRINT
    DPRINTLN("Failed parsing settings JSON. Out of memory?");
    #endif
  }

  // Deallocate buffer
  if (buf != NULL) delete[] buf;

  return status;
}

bool get_settings_from_web_server(ModuleInterfaceSet &interfaces, Client &client, uint8_t *server, uint8_t port = 80) {
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
    else { DPRINT("Connection to web server failed with code "); DPRINTLN(code); }
    #endif
    client.stop();
    if (success) cnt++;
  }
  #ifdef DEBUG_PRINT
  DPRINT(F("Reading settings took ")); DPRINT((uint32_t)(millis() - start_time)); DPRINTLN("ms.");
  #endif
  return cnt > 0;
}

void post_json_to_server(Client &client, const String &buf, const String &request) {
#ifdef MI_SMALLMEM
  // This is to minimize memory usage on the Arduinos
  String head = String(F("POST /")) + request + String(F(" HTTP/1.0\r\n"
    "Content-Type: application/json\r\n"
    "Connection: close\r\n"
    "Content-Length: "));
  client.print(head);
  client.println(buf.length());
  client.println();
  client.println(buf);
  client.println();
#else
  // If we have more memory available, speed up the transfer by doing one write only
  char buflen[10];
  _itoa(buf.length(), buflen, 10);
  String s = F("POST /") + request + F(" HTTP/1.0\r\n"
    "Content-Type: application/json\r\n"
    "Connection: close\r\n"
    "Content-Length: ") + buflen
    + "\r\n\r\n" + buf + "\r\n\r\n";  
  client.print(s.c_str());
#endif  
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

void add_json_settings(ModuleInterface *interface, JsonObject &root) {
  
  if (!interface->settings.got_contract() || !interface->settings.is_updated()) return; // Values not available yet

  // Add output values
  char prefixed_name[MVAR_MAX_NAME_LENGTH + MVAR_PREFIX_LENGTH + 1];
  for (int i=0; i<interface->settings.get_num_variables(); i++) {
    if (interface->settings.get_module_variable(i).is_changed()) {
      interface->settings.get_module_variable(i).get_prefixed_name(interface->get_prefix(), prefixed_name, sizeof prefixed_name);
      mv_to_json(interface->settings.get_module_variable(i), root, prefixed_name);
    }
  }
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
  DPRINT(F("MEM Free=")); DPRINT(total_free); DPRINT(F(", #frag=")); DPRINT(num_fragments);
  DPRINT(", bigfrag="); DPRINTLN(largest_free);
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

#ifdef MI_SMALLMEM // Little memory, transfer values for each module in separate requests.

bool send_values_to_web_server(ModuleInterfaceSet &interfaces, Client &client, const uint8_t *server_ip,
                               MILastScanTimes *last_scan_times, uint8_t port = 80,
                               bool primary_master = true, // (set primary_master=false on all masters but one if more than one)
                               uint16_t json_buffer_size = 500) {
  int successCnt = 0;
  #ifdef DEBUG_PRINT
  uint32_t start_time = millis();
  #endif
  IPAddress server(server_ip);
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
    //DPRINTLN(buf);
    DPRINT(F("Writing ")); DPRINT(buf.length()); DPRINT(F(" bytes (outputs) to web server for "));
    DPRINTLN(interfaces[i]->module_name);
    #endif

    // Post JSON to web server
    int8_t code = client.connect(server, port);
    if (code == 1) { // 1=CONNECTED
      post_json_to_server(client, buf, String(F("set_currentvalues.php")));
    }
    #ifdef DEBUG_PRINT
    else { DPRINT(F("Connection to web server failed with code ")); DPRINTLN(code); }
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
    else { DPRINT(F("Connection to web server failed with code ")); DPRINTLN(code); }
    #endif
    client.stop();
  }
  
  #ifdef DEBUG_PRINT
  DPRINT(F("Writing to web server took ")); DPRINT((uint32_t)(millis() - start_time));
  DPRINTLN(F("ms."));
  #endif
  return successCnt > 0;
}

#else // More memory, send values for all modules in one request (faster)

bool send_values_to_web_server(ModuleInterfaceSet &interfaces, Client &client, const uint8_t *server,
                               MILastScanTimes *last_scan_times, uint8_t port = 80,
                               bool primary_master = true, // (set primary_master=false on all masters but one if more than one)
                               uint16_t json_buffer_size = 3000) {
  int successCnt = 0;
  #ifdef DEBUG_PRINT
  uint32_t start_time = millis();
  #endif
  String buf;  
  { // Separate block to release JSON buffer as soon as possible
    DynamicJsonBuffer jsonBuffer(json_buffer_size);
    JsonObject& root = jsonBuffer.createObject();

    // Set scan columns in database if inserting (support for plotting with different resolutions)
    if (primary_master) set_scan_columns(root, last_scan_times);

    // Add status for the master
    add_master_status(interfaces, root);
    
    // Add values from all modules
    for (int i=0; i<interfaces.num_interfaces; i++) add_json_values(interfaces[i], root);
    
    // Encode all settings to a JSON string
    root.printTo(buf);
  }
  
  #ifdef DEBUG_PRINT
  //DPRINTLN(buf);
  DPRINT(F("Writing ")); DPRINT(buf.length()); DPRINTLN(F(" bytes (outputs) to web server"));
  #endif

  // Post JSON to web server
  int8_t code = client.connect(server, port);
  if (code == 1) { // 1=CONNECTED
    post_json_to_server(client, buf, String(F("set_currentvalues.php")));
  }
  #ifdef DEBUG_PRINT
  else { DPRINT(F("Connection to web server failed with code ")); DPRINTLN(code); }
  #endif
  client.stop();
  if (code == 1) successCnt++;
  
  // Take a snapshot of the currentvalues table into the timeseries table
  if (primary_master) {
    int8_t code = client.connect(server, port);
    if (code == 1) { // 1=CONNECTED
      String s = F("GET /store_currentvalues.php");
      s += "\r\n";
      client.print(s.c_str());
    }
    #ifdef DEBUG_PRINT
    else { DPRINT(F("Connection to web server failed with code ")); DPRINTLN(code); }
    #endif
    client.stop();
  }
  
  #ifdef DEBUG_PRINT
  DPRINT(F("Writing to web server took ")); DPRINT((uint32_t)(millis() - start_time));
  DPRINTLN(F("ms."));
  #endif
  return successCnt > 0;
}

#endif

bool send_settings_to_web_server(ModuleInterfaceSet &interfaces, Client &client, const uint8_t *server,
                               uint8_t port = 80, uint16_t json_buffer_size = 3000) {
  // Quick check if there is anything to do                               
  bool changes = false;
  for (int i=0; i<interfaces.num_interfaces; i++) changes = changes || interfaces[i]->settings.is_changed();
  if (!changes) return false;  

  int successCnt = 0;
  #ifdef DEBUG_PRINT
  uint32_t start_time = millis();
  #endif
  String buf;  
  { // Separate block to release JSON buffer as soon as possible
    DynamicJsonBuffer jsonBuffer(json_buffer_size);
    JsonObject& root = jsonBuffer.createObject();
    
    // Add values from all modules
    for (int i=0; i<interfaces.num_interfaces; i++) add_json_settings(interfaces[i], root);
    
    // Encode all settings to a JSON string
    root.printTo(buf);
  }
  
  #ifdef DEBUG_PRINT
  DPRINTLN(buf.c_str());
  DPRINT(F("REVERSE writing ")); DPRINT(buf.length()); DPRINTLN(F(" bytes (settings) to web server"));
  #endif
  if (buf.length() == 0) return false; // No changed settings
  
  // Post JSON to web server
  int8_t code = client.connect(server, port);
  if (code == 1) { // 1=CONNECTED
    post_json_to_server(client, buf, String(F("set_settings.php")));
  }
  #ifdef DEBUG_PRINT
  else { DPRINT(F("Connection to web server failed with code ")); DPRINTLN(code); }
  #endif
  client.stop();
  if (code == 1) successCnt++;   
  return successCnt > 0;
}

class MIHttpTransfer {
protected:  
  // Configuration
  uint32_t settings_interval,
           outputs_interval;
  ModuleInterfaceSet &interfaces;
  uint8_t web_server_ip[4];
  
  // State
  uint32_t last_settings = 0, last_outputs = millis();
  MILastScanTimes last_scan_times;
  Client &client;

public:
  MIHttpTransfer(ModuleInterfaceSet &module_interface_set,
                 Client &web_client,
                 const uint8_t *web_server_address,
                 const uint32_t settings_transfer_interval_ms = 10000, 
                 const uint32_t outputs_transfer_interval_ms  = 10000) : 
                 interfaces(module_interface_set), client(web_client) {
    if (web_server_address) memcpy(web_server_ip, web_server_address, 4);
    settings_interval = settings_transfer_interval_ms;
    outputs_interval = outputs_transfer_interval_ms;                   
  }
  
  void set_web_server_address(const uint8_t *server_address) { memcpy(web_server_ip, server_address, 4); }
 
  void update() {
    // Get settings for each module from the database via the web server
    if (mi_interval_elapsed(last_settings, settings_interval)) {
      send_settings_to_web_server(interfaces, client, web_server_ip);
      get_settings_from_web_server(interfaces, client, web_server_ip);
    }

    // Store all measurements to the database via the web server
    if (mi_interval_elapsed(last_outputs, outputs_interval)) {
      // (set primary_master=false on all masters but one if there are more than one)
      send_values_to_web_server(interfaces, client, web_server_ip, &last_scan_times); 
    }    
  }  
};
