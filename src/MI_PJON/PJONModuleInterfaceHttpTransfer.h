#pragma once

#include <MI/ModuleInterfaceHttpTransfer.h>

// Get and activate master settings from a parsed json document
bool read_master_json_settings(PJONModuleInterfaceSet &interfaces, DynamicJsonDocument &root, DeserializationError &error) {
  bool status = false;
  if (!error) {
    String key = interfaces.get_prefix(); key += "DevID";
    uint8_t device_id = (uint8_t) root[key];
    if (device_id != 0) interfaces.get_link()->set_id(device_id);

    key = interfaces.get_prefix(); key += "Modules";
    String module_list = (const char*)root[key];
    interfaces.set_interface_list(module_list.c_str());
    #ifdef DEBUG_PRINT
    DPRINT("Modules: '"); DPRINT(module_list.c_str()); DPRINTLN("'");
    #endif

    uint32_t interval = interfaces.get_transfer_interval();
    key = interfaces.get_prefix(); key += "IntSettings";
    uint32_t t = (uint32_t)root[key];
    if (t != 0) interval = t;

    key = interfaces.get_prefix(); key += "IntOutputs";
    t = (uint32_t)root[key];
    if (t != 0) interval = t;
    
    interfaces.set_transfer_interval(interval);

    status = module_list.length() > 5;
  } else {
    #ifdef DEBUG_PRINT
    DPRINT("Failed parsing master settings JSON. Out of memory?: ");
    DPRINTLN(error.c_str());
    #endif
  }
  return status;
}

// Read, parse and activate master settings from a connected HTTP client
bool read_master_json_settings(PJONModuleInterfaceSet &interfaces, 
                               Client &client,
                               const uint16_t buffer_size = MI_MAX_JSON_SIZE, 
                               const uint16_t timeout_ms = 3000)
{
  char *buf = new char[buffer_size];
  DynamicJsonDocument root(buffer_size);
  auto error = read_json_settings_from_server(client, root, buf, buffer_size, timeout_ms);
  bool status = read_master_json_settings(interfaces, root, error);

  // Deallocate buffer
  if (buf != NULL) delete[] buf;

  return status;
}

// Connect to a HTTP server, request, read, parse and activate master settings
bool get_master_settings_from_web_server(PJONModuleInterfaceSet &interfaces, 
                                         Client &client, 
                                         uint8_t *server, 
                                         uint16_t port = 80)
{
  int8_t code = client.connect(server, port);
  bool success = false;
  if (code == 1) { // 1=CONNECTED
    write_http_settings_request(interfaces.get_prefix(), client);
    success = read_master_json_settings(interfaces, client);
  }
  #ifdef DEBUG_PRINT
  else { DPRINT("Connection to web server failed with code "); DPRINTLN(code); }
  #endif
  client.stop();
  return success;
}

class PJONMIHttpTransfer : public MIHttpTransfer {
public:
  PJONMIHttpTransfer(PJONModuleInterfaceSet &module_interface_set,
                 Client &web_client,
                 const uint8_t *web_server_address) : 
                 MIHttpTransfer(module_interface_set, web_client, web_server_address) { }
  
  bool get_master_settings_from_server() {
    // Read the module list and other settings from the web server
    return get_master_settings_from_web_server(*(PJONModuleInterfaceSet*)&interfaces, client, web_server_ip, web_server_port);
  }
};
