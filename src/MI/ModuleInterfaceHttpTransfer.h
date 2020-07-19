#pragma once

#include <MI/ModuleInterface.h>
#include <MI/MITransferBase.h>
#include <utils/MITime.h>
#include <utils/MIUptime.h>
#include <utils/MemFrag.h>

#include <ArduinoJson.h>

// Include correct definition of the Client class
#include <strategies/EthernetTCP/EthernetTCP.h>

// See if we have to minimize memory usage by splitting operations into smaller parts
#if defined(ARDUINO) && !defined(PJON_ESP)
#define MI_SMALLMEM
#endif

// A buffer is used for transferring JSON data, and the max size can be defined here
#ifndef MI_MAX_JSON_SIZE
  #ifdef MI_SMALLMEM
    #define MI_MAX_JSON_SIZE 800
  #else
    #define MI_MAX_JSON_SIZE 15000
  #endif
#endif

void write_http_settings_request(const char *module_prefix, Client &client) {
  String request = F("GET /get_settings.php");
  if (module_prefix != NULL) {
    request += F("?prefix=");
    request += module_prefix;
  }
  request += "\r\n\r\n";
  client.print(request.c_str());
  // NOTE: On ESP8266 a client.flush() seems to close the connection, so avoid it
}

template <class T> void val_to_buf(uint32_t *buf, T value) { memcpy(buf, &value, sizeof value); }

void buf_to_mvar(ModuleVariable &v, const uint32_t *value, const uint8_t size) {
  MITransferBase::set_mv_and_changed_flags(v, value, size, 0); // HTTP transfer MUST be the first if multiple!
}


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

bool json_to_mv(ModuleVariable &v, DynamicJsonDocument& root, const char *name) {
  JsonObject obj = root.as<JsonObject>();
  return json_to_mv(v, obj, name);
}

bool mv_to_json(const ModuleVariable &v, JsonObject& parent, const char *name_in) {
  String name = name_in; // For some reason a char* will succeed but be forgotten. Probably added as a pointer in JSON enocder
  const float SYS_ZERO = -999.25; // Marker for missing value used in some proprietary systems
  switch (v.get_type()) {
  case mvtBoolean: parent[name] = (uint8_t)(v.get_bool() ? 1 : 0); return true;
  case mvtUint8: parent[name] = v.get_uint8(); return true;
  case mvtInt8: parent[name] = v.get_int8(); return true;
  case mvtUint16: parent[name] = v.get_uint16(); return true;
  case mvtInt16: parent[name] = v.get_int16(); return true;
  case mvtUint32: parent[name] = v.get_uint32(); return true;
  case mvtInt32: parent[name] = v.get_int32(); return true;
  case mvtFloat32:
    if (v.get_float() != SYS_ZERO && !isnan(v.get_float()) && !isinf(v.get_float())) {
      parent[name] = v.get_float(); return true;
    }
    else return false;
  case mvtUnknown: return false;
  }
  return false;
}

bool mv_to_json(const ModuleVariable &v, DynamicJsonDocument& root, const char *name_in) {
  JsonObject obj = root.as<JsonObject>();
  return mv_to_json(v, obj, name_in);
}

void set_time_from_json(DynamicJsonDocument& root, uint32_t delay_ms) {
 // Set time if received (as early as possible)
  uint32_t utc = (uint32_t)root["UTC"];
  if (utc != 0) {
    utc += delay_ms/1000ul;
    if (abs((int32_t)(utc -miTime::Get()) > 2)) {
    #ifndef NO_TIME_SYNC
      #ifdef DEBUG_PRINT
        DPRINT(F("--> Adjusted time from web by s: ")); DPRINTLN((int32_t) (utc - miTime::Get()));
      #endif
      miTime::Set(utc); // Set system time
    #endif
    }
  }
  #ifdef DEBUG_PRINT
  else DPRINTLN(F("Got no time from web server!"));
  #endif
}

void decode_json_settings(ModuleInterface &interface, DynamicJsonDocument& root) {
  if (!interface.settings.got_contract()) return;

  char prefixed_name[MVAR_MAX_NAME_LENGTH + MVAR_PREFIX_LENGTH + 1];
  for (int j=0; j<interface.settings.get_num_variables(); j++) {
    interface.settings.get_module_variable(j).get_prefixed_name(interface.get_prefix(), prefixed_name, sizeof prefixed_name);
    #if defined(MASTER_MULTI_TRANSFER) && defined(DEBUG_PRINT_SETTINGSYNC)
    ModuleVariable &mv = interface.settings.get_module_variable(j);
    bool prev_changed = mv.is_changed();
    uint8_t prev_bits = mv.change_bits;
    #endif
    json_to_mv(interface.settings.get_module_variable(j), root, prefixed_name);
    #if defined(MASTER_MULTI_TRANSFER) && defined(DEBUG_PRINT_SETTINGSYNC)
    if (prev_bits != mv.change_bits) 
      printf("FROM HTML '%s' VAL %ld cbits: %d->%d changed:%d->%d\n", prefixed_name, mv.get_uint32(), 
        prev_bits, mv.change_bits, prev_changed, mv.is_changed());
     #endif
  }
  interface.settings.set_updated(); // Flag that settings are ready to be used
}

DeserializationError read_json_settings_from_server(
    Client &client, DynamicJsonDocument &root, char *buf, const uint16_t buffer_size, 
    const uint16_t timeout_ms = 3000) 
{
  if (buf == NULL) {
    mvs_out_of_memory = true;
    #ifdef DEBUG_PRINT
    DPRINTLN(F("read_json_settings OUT OF MEMORY"));
    #endif
    return DeserializationError::NoMemory;
  }
  uint16_t pos = 0;
  uint32_t start = millis();
  while ((client.connected() || client.available()) && (uint32_t)(millis()-start) < timeout_ms) {
    if (pos >= buffer_size -1) break;
    if (client.available()) {
      uint16_t len = client.read((uint8_t*) &buf[pos], buffer_size - pos -1); 
      if (len > 0) pos += len; else break;
    } else delay(1);
  }
  buf[pos] = 0; // null-terminate
  client.stop();  // Finished using the socket, close it as soon as possible
  char *jsonStart = buf;
  if (pos >= buffer_size - 1) {
    mvs_out_of_memory = true;
    #ifdef DEBUG_PRINT
    DPRINT(pos); DPRINTLN(F(" bytes, read_json_settings BUFFER TOO SMALL"));
    #endif
    return DeserializationError::NoMemory;
  }
  if (pos > 0) {
    // Locate JSON part of reply
    const char *jsonDecl = "Content-Type: application/json";
    jsonStart = strstr(buf, jsonDecl);
    if (jsonStart) jsonStart += strlen(jsonDecl) + 1;
    else jsonStart = buf;
  }
  return deserializeJson(root, jsonStart);
}

#ifdef MI_SMALLMEM
bool read_json_settings(ModuleInterface &interface, Client &client, 
                        const uint16_t buffer_size = MI_MAX_JSON_SIZE, const uint16_t timeout_ms = 3000) 
{
  char *buf = new char[buffer_size];
  DynamicJsonDocument root(buffer_size);
  uint32_t start = millis();
  auto error = read_json_settings_from_server(client, root, buf, buffer_size, timeout_ms);
  bool status = false;
  if (!error) {
    // Set system time if UTC was returned from server, exclude parsing time and half of retrieval time
    #ifdef DEBUG_PRINT
    uint32_t before_set = millis();
    #endif
    set_time_from_json(root, (uint32_t)(millis()-start));
    decode_json_settings(interface, root);
    status = true;
  } else {
    #ifdef DEBUG_PRINT
    DPRINT("Failed parsing settings JSON. Out of memory(1)?: ");
    DPRINTLN(error.c_str());
    #endif
  }

  // Deallocate buffer
  if (buf != NULL) delete[] buf;

  return status;
}
#else
bool read_json_settings(ModuleInterfaceSet &interfaces, Client &client,
  const uint16_t buffer_size_per_module = MI_MAX_JSON_SIZE, const uint16_t timeout_ms = 3000)
{
  if (interfaces.num_interfaces == 0) return false;
  const uint32_t buffer_size = buffer_size_per_module * interfaces.num_interfaces;
  char *buf = new char[buffer_size];
  DynamicJsonDocument root(buffer_size);
  uint32_t start = millis();
  auto error = read_json_settings_from_server(client, root, buf, buffer_size, timeout_ms);
  bool status = false;
  if (!error) {
    // Set system time if UTC was returned from server, exclude parsing time and half of retrieval time
    #ifdef DEBUG_PRINT
    uint32_t before_set = millis();
    #endif
    set_time_from_json(root, (uint32_t)(millis() - start));
    for (int i = 0; i < interfaces.num_interfaces; i++) {
      decode_json_settings(*interfaces[i], root);
    }
    status = true;
  }
  else {
    #ifdef DEBUG_PRINT
    DPRINT("Failed parsing settings JSON. Out of memory(2)?: ");
    DPRINTLN(error.c_str());
    #endif
  }

  // Deallocate buffer
  if (buf != NULL) delete[] buf;

  return status;
}
#endif

bool get_settings_from_web_server(ModuleInterfaceSet &interfaces, Client &client, uint8_t *server, uint16_t port = 80) {
  #ifdef DEBUG_PRINT
  uint32_t start_time = millis();
  #endif
  uint8_t cnt = 0;
#ifdef MI_SMALLMEM
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
#else
  int8_t code = client.connect(server, port);
  bool success = false;
  if (code == 1) { // 1=CONNECTED
    write_http_settings_request(NULL, client);
    success = read_json_settings(interfaces, client);
  }
  #ifdef DEBUG_PRINT
  else { DPRINT("Connection to web server failed with code "); DPRINTLN(code); }
  #endif
  client.stop();
  if (success) cnt++;
#endif
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

uint32_t difftime(const uint32_t start, const uint32_t end) {
  if (start == 0 || end == 0) return 0;
  uint32_t diff = (uint32_t)(end - start);
  return diff > 600000 ? 0 : diff;
}

uint32_t get_max_request_time(ModuleInterface *interface) {
  // Find max time of request of outputs, settings or status
  uint32_t statustime = difftime(interface->before_status_requested_time, interface->status_received_time),
           outputtime = difftime(interface->outputs.before_requested_time, interface->outputs.get_updated_time_ms()),
           maxtime = statustime > outputtime ? statustime : outputtime;

  // NOTE: Settings are requested when the module flags that is has modified settings.
  // Otherwise settings are retrived from the web server, not the module.
  // Therefore we cannot use the time from request is sent until modification time because
  // request time will stay static and modified time will be updated regularly.
  return maxtime;
}

void add_module_status(ModuleInterface *interface, DynamicJsonDocument &root) {
  // Add status values
  String name;
  int16_t age = interface->get_last_alive_age();
  if (age >= 0 && miTime::Get()!=0) { // Leave last registered UTC value in database if unknown alive age
    name = interface->get_prefix(); name += F("LastLife");
    root[name] = (uint32_t)(miTime::Get() - age); // Set as UTC
  }

  name = interface->get_prefix(); name += F("Uptime");
  root[name] = interface->get_uptime_s();

  name = interface->get_prefix(); name += F("MemErr");
  root[name] = (uint8_t) interface->out_of_memory;

  name = interface->get_prefix(); name += F("StatBits");
  root[name] = (uint8_t) interface->get_status_bits();

  // Find max time of request of outputs, settings or status
  name = interface->get_prefix(); name += F("ReqTime");
  root[name] = get_max_request_time(interface);
}

void add_json_values(ModuleInterface *interface, DynamicJsonDocument &root) {
  
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

void add_json_settings(ModuleInterface *interface, DynamicJsonDocument &root) {
  
  if (!interface->settings.got_contract() || !interface->settings.is_updated()) return; // Values not available yet

  // Add output values
  char prefixed_name[MVAR_MAX_NAME_LENGTH + MVAR_PREFIX_LENGTH + 1];
  for (int i=0; i<interface->settings.get_num_variables(); i++) {
    if (MITransferBase::is_mv_changed(interface->settings.get_module_variable(i), 0)) {
      interface->settings.get_module_variable(i).get_prefixed_name(interface->get_prefix(), prefixed_name, sizeof prefixed_name);
      #if defined(MASTER_MULTI_TRANSFER) && defined(DEBUG_PRINT_SETTINGSYNC)
      ModuleVariable &mv = interface->settings.get_module_variable(i);
      bool prev_changed = mv.is_changed();
      uint8_t prev_bits = mv.change_bits;
      #endif
      mv_to_json(interface->settings.get_module_variable(i), root, prefixed_name);
      #if defined(MASTER_MULTI_TRANSFER) && defined(DEBUG_PRINT_SETTINGSYNC)
      if (MITransferBase::is_mv_changed(mv, 0)) 
        printf("TO HTML '%s' VAL %ld cbits: %d->%d changed:%d->%d\n", prefixed_name, mv.get_uint32(), 
          prev_bits, mv.change_bits, prev_changed, mv.is_changed());
      #endif
    }
  }
}

void set_scan_columns(DynamicJsonDocument &root,
                      MILastScanTimes *last_scan_times)
{
  if (last_scan_times) {
    // Do not sample at startup, init time array to now
    uint32_t curr = miTime::Get();
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

void FindMaxRequestTime(ModuleInterfaceSet &interfaces, uint32_t &req_time, uint8_t &req_ix) {
  req_time = 0;
  req_ix = 255;
  for (uint8_t i=0; i<interfaces.num_interfaces; i++) {
    uint32_t devicemaxtime = get_max_request_time(interfaces[i]);
    if (devicemaxtime > req_time) {
      req_time = devicemaxtime;
      req_ix = i;
    }
  }
}

void add_master_status(ModuleInterfaceSet &interfaces, DynamicJsonDocument &root, const MILastScanTimes &last_scan_times) {
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
  root[name] = (uint8_t) mvs_out_of_memory;
  
  // Add uptime
  name = interfaces.get_prefix(); name += F("Uptime");
  root[name] = (uint32_t) miGetUptime();
  
  // Add system time
  name = interfaces.get_prefix(); name += F("UTC");
  root[name] = (uint32_t) miTime::Get();

  // Add times spent in HTTP requests
  name = interfaces.get_prefix(); name += F("WValTm"); // Write values time
  root[name] = last_scan_times.last_set_values_usage_ms;

  name = interfaces.get_prefix(); name += F("WSetTm"); // Write settings time
  root[name] = last_scan_times.last_set_settings_usage_ms;

  name = interfaces.get_prefix(); name += F("RSetTm"); // Read settings time
  root[name] = last_scan_times.last_get_settings_usage_ms;

  // Register the longest request time and the responsible module
  uint32_t req_time = 0;
  uint8_t req_ix = 255;
  FindMaxRequestTime(interfaces, req_time, req_ix);
  name = interfaces.get_prefix(); name += F("MaxReqTm"); // Maximum response time across all modules
  root[name] = req_time;
  name = interfaces.get_prefix(); name += F("MaxReqIx"); // Array ix of module with max response time
  root[name] = req_ix;

  // Total time spent in timed transfer for the last time
  // (Settings and outputs/inputs between modules and between modules and web server)
  name = interfaces.get_prefix(); name += F("TotalTm"); // Total transfer time
  root[name] = interfaces.last_total_usage_ms;
}

#ifdef MI_SMALLMEM // Little memory, transfer values for each module in separate requests.

bool send_values_to_web_server(ModuleInterfaceSet &interfaces, Client &client, const uint8_t *server_ip,
                               MILastScanTimes *last_scan_times, uint16_t port = 80,
                               bool primary_master = true, // (set primary_master=false on all masters but one if more than one)
                               uint16_t json_buffer_size = MI_MAX_JSON_SIZE) {
  int successCnt = 0;
  #ifdef DEBUG_PRINT
  uint32_t start_time = millis();
  #endif
  IPAddress server(server_ip);
  for (int i=0; i<interfaces.num_interfaces; i++) {
    // Encode all settings to a JSON string
    String buf;
    { // Separate block to release JSON buffer as soon as possible
      DynamicJsonDocument root(json_buffer_size);
      add_json_values(interfaces[i], root);

      // Set scan columns in database if inserting (support for plotting with different resolutions)
      if (primary_master && i == 0) set_scan_columns(root, last_scan_times);

      // Add status for the master
      if (i == 0) add_master_status(interfaces, root, *last_scan_times);
      serializeJson(root, buf);
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
                               MILastScanTimes *last_scan_times, uint16_t port = 80,
                               bool primary_master = true, // (set primary_master=false on all masters but one if more than one)
                               uint16_t json_buffer_size = MI_MAX_JSON_SIZE) {
  int successCnt = 0;
  #ifdef DEBUG_PRINT
  uint32_t start_time = millis();
  #endif
  String buf;  
  { // Separate block to release JSON buffer as soon as possible
    DynamicJsonDocument root(json_buffer_size);

    // Set scan columns in database if inserting (support for plotting with different resolutions)
    if (primary_master) set_scan_columns(root, last_scan_times);

    // Add status for the master
    add_master_status(interfaces, root, *last_scan_times);
    
    // Add values from all modules
    for (int i=0; i<interfaces.num_interfaces; i++) add_json_values(interfaces[i], root);
    
    // Encode all settings to a JSON string
    serializeJson(root, buf);
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
                               uint16_t port = 80, uint16_t json_buffer_size = MI_MAX_JSON_SIZE) {
  // Quick check if there is anything to do                               
  bool changes = false;
  for (int i=0; i<interfaces.num_interfaces; i++) changes = changes || MITransferBase::is_mvs_changed(interfaces[i]->settings, 0);
  if (!changes) return false;  

  int successCnt = 0;
  #ifdef DEBUG_PRINT
  uint32_t start_time = millis();
  #endif
  String buf;  
  { // Separate block to release JSON buffer as soon as possible
    DynamicJsonDocument root(json_buffer_size);
    
    // Add values from all modules
    root["Updated"] = (uint32_t)millis(); // With ArduinoJson 6 it is necessary to set something here, otherwise mv_to_json will fail O_O
    for (int i=0; i<interfaces.num_interfaces; i++) add_json_settings(interfaces[i], root);
    
    // Encode all settings to a JSON string
    serializeJson(root, buf);
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

class MIHttpTransfer : public MITransferBase {
protected:  
  // Configuration
  uint8_t web_server_ip[4];
  uint16_t web_server_port = 80;
  bool is_primary_master = true;
  
  // State
  Client &client;

public:
  MIHttpTransfer(ModuleInterfaceSet &module_interface_set,
                 Client &web_client,
                 const uint8_t *web_server_address) : 
                 MITransferBase(module_interface_set),
                 client(web_client) {
    if (web_server_address) memcpy(web_server_ip, web_server_address, 4);
  }
  
  void update() {}

  void set_web_server_address(const uint8_t *server_address) { memcpy(web_server_ip, server_address, 4); }

  void set_web_server_port(uint16_t server_port) { web_server_port = server_port; }

  void set_primary_master(bool is_primary) { is_primary_master = is_primary; }

  void get_settings() {
    uint32_t start = millis();
    get_settings_from_web_server(interfaces, client, web_server_ip, web_server_port);
    last_scan_times.last_get_settings_usage_ms = (uint32_t)(millis() - start);
  }

  void put_settings() {
    // If any setting has been modified in module, send it to the web server
    uint32_t start = millis();
    send_settings_to_web_server(interfaces, client, web_server_ip, web_server_port);
    last_scan_times.last_set_settings_usage_ms = (uint32_t)(millis() - start);
  }

  void get_values() {} // Getting values (inputs) from web server not supported for now

  void put_values() {
    // Send values (outputs) to the web server
    uint32_t start = millis();
    send_values_to_web_server(interfaces, client, web_server_ip, &last_scan_times,
      web_server_port, is_primary_master);
    last_scan_times.last_set_values_usage_ms = (uint32_t)(millis() - start);
  }  
};
