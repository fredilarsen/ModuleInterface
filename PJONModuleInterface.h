#pragma once

#include <ModuleInterface.h>

#ifndef MAX_PACKETS
  #define MAX_PACKETS 1
#endif
#ifndef PACKET_MAX_LENGTH
  #define PACKET_MAX_LENGTH 250
#endif

#include <Link.h>

// A timeout to make sure a lost request or reply does not stop everything permanently
#define MI_REQUEST_TIMEOUT 5000000   // (us) How long to wait for an active module to reply to a request
#define MI_SEND_TIMEOUT 5000000      // (us) How long to wait for an active device to ACK
#define MI_REDUCED_SEND_TIMEOUT 5000 // (us) How long to try to contact a moduled that is marked as inactive
#define MI_INACTIVE_THRESHOLD 5      // The number of consecutive failed transfers before module is deactivated

// A status bit for the PJON extended header byte, used to quickly separate ModuleInterface related messages from others
#define MI_PJON_BIT SESSION_BIT

class PJONModuleInterface : public ModuleInterface {
friend class PJONModuleInterfaceSet;  
protected:
  Link *pjon = NULL;
  #ifdef IS_MASTER
  uint8_t remote_id = 0;
  uint8_t remote_bus_id[4];
  uint32_t status_requested_time = 0;
  #else
  static PJONModuleInterface *singleton;
  void set_link(Link &pjon) { this->pjon = &pjon; singleton = this; pjon.set_receiver(default_receiver_function); }
  #endif
public:
  // Constructors for Master side
  #ifdef IS_MASTER
  PJONModuleInterface() { init(); }
  PJONModuleInterface(const char *module_name_prefix_and_address) { 
    init();
    set_name_prefix_and_address(module_name_prefix_and_address);
  }
  PJONModuleInterface(const char *module_name, const char *prefix, Link &pjon, uint8_t remote_id, uint8_t remote_bus_id[])
    : ModuleInterface(module_name, prefix) {
    init(); set_remote_device(pjon, remote_id, remote_bus_id);
  }
  #endif

  // Constructors for Module side
  #ifndef IS_MASTER  
  PJONModuleInterface(const char *module_name, Link &pjon, 
                      const char *settingnames, const char *inputnames, const char *outputnames) :
    ModuleInterface(module_name, settingnames, inputnames, outputnames) {      
    set_link(pjon);
  }
  // This constructor is available only because the F() macro can only be used in a function.
  // So if RAM is tight, use this constructor and call set_contracts from setup()
  PJONModuleInterface(Link &pjon, const uint8_t num_settings, const uint8_t num_inputs, const uint8_t num_outputs) :
    ModuleInterface(num_settings, num_inputs, num_outputs) {
    set_link(pjon);
  }
  PJONModuleInterface(Link &pjon) : ModuleInterface() { set_link(pjon); }
  
  static PJONModuleInterface *get_singleton() { return singleton; }
  #endif 
  
  void init() {
    ModuleInterface::init();    
    #ifdef IS_MASTER
      memset(remote_bus_id, 0, 4);
    #endif
  }

  void set_bus(Link &bus) { pjon = &bus; }
  
  void update() {
    pjon->receive(1000);
    pjon->update();
  }

  // Whether this module is active or has not been reachable for a while
  bool is_active() const { 
    #ifdef IS_MASTER
      return comm_failures < MI_INACTIVE_THRESHOLD;
    #else
      return true;
    #endif  
  }
  
  #ifdef IS_MASTER  
  void set_remote_device(Link &pjon, uint8_t remote_id, uint8_t remote_bus_id[]) {
    this->pjon = &pjon; this->remote_id = remote_id; memcpy(this->remote_bus_id, remote_bus_id, 4);
  }

  void set_name_prefix_and_address(const char *name_and_address) { // Format like "device1:d1:44" or "device1:d1:44:0.0.0.1"
    // Split input string into name, device id and bus id
    const char *end = strstr(name_and_address, " ");
    if (!end) end = name_and_address + strlen(name_and_address);           
    uint8_t len = end - name_and_address;
    char *buf = new char[len + 1];
    if (buf == NULL) { ModuleVariableSet::out_of_memory = true; return; }
    memcpy(buf, name_and_address, len); buf[len] = 0;
    char *p1 = strstr(buf + 1, ":");
    if (p1) { *p1 = 0; p1++; } // p1 now pointing to prefix
    char *p2 = p1 != NULL ? strstr(p1 + 1, ":") : NULL;
    if (p2) { *p2 = 0; p2++; } // p2 now pointing to device id
    char *p3 = p2 != NULL ? strstr(p2 + 1, ":") : NULL;
    if (p3) { *p3 = 0; p3++; } // p3 now pointing to bus id        
    // Set the values
    set_name(buf);
    if (p1) set_prefix(p1);
    if (p2) remote_id = atoi(p2);
    if (p3) {
      for (int i=0; i<4; i++) {
        remote_bus_id[i] = atoi(p3);
        while (*p3 != '.' && *p3 != 0) p3++;
        if (*p3 != '.') break;
        p3++;
      }
    }
    delete buf;
  }
   
  void update_contract(const uint32_t interval_ms) {
    if (!settings.got_contract() && (settings.contract_requested_time == 0 || (millis()-settings.contract_requested_time >= interval_ms))) {
      if (send_setting_contract_request()) pjon->receive(MI_REQUEST_TIMEOUT);
    }
    if (!inputs.got_contract() && (inputs.contract_requested_time == 0 || (millis()-inputs.contract_requested_time >= interval_ms))) {
      if (send_input_contract_request()) pjon->receive(MI_REQUEST_TIMEOUT);
    }
    if (!outputs.got_contract() && (outputs.contract_requested_time == 0 || (millis()-outputs.contract_requested_time >= interval_ms))) {
      if (send_output_contract_request()) pjon->receive(MI_REQUEST_TIMEOUT);
    }
  }

  void update_values(const uint32_t interval_ms) {
    if (outputs.got_contract() && (outputs.requested_time == 0 || (millis()-outputs.requested_time >= interval_ms))) {
      if (send_outputs_request()) pjon->receive(MI_REQUEST_TIMEOUT);
    }
  }

  void update_status(const uint32_t interval_ms) {
    if (got_contract() && (status_requested_time == 0 || (millis()-status_requested_time >= interval_ms))) {
      if (send_status_request()) pjon->receive(MI_REQUEST_TIMEOUT);
    }
  }
  
  // Sending of data from master to remote module
  void send_settings() {
    #ifdef DEBUG_PRINT
    if (settings.got_contract() && !settings.is_updated()) Serial.println(F("Settings not sent because not updated yet."));
    #endif
    if (!settings.got_contract() || !settings.is_updated()) return;
    notify(ntSampleSettings, this);
    BinaryBuffer response;
    uint8_t response_length = 0;
    settings.get_values(response, response_length, mcSetSettings);    
    send(remote_id, remote_bus_id, response.get(), response_length); 
  }
  
  void send_inputs() { 
    #ifdef DEBUG_PRINT
    if (inputs.got_contract() && !inputs.is_updated() && inputs.get_num_variables()>0) Serial.println(F("Inputs not sent because not updated yet."));
    #endif
    if (!inputs.got_contract() || !inputs.is_updated()) return;
    notify(ntSampleInputs, this);
    BinaryBuffer response;
    uint8_t response_length = 0;
    inputs.get_values(response, response_length, mcSetInputs);
    send(remote_id, remote_bus_id, response.get(), response_length); 
  }
    
  // Sending of requests  
  bool send_cmd(const uint8_t &value) {
    #ifdef DEBUG_PRINT
      Serial.print(F("SENT REQUEST ")); Serial.println(value);
    #endif 
    return send(remote_id, remote_bus_id, &value, 1);
  }
  bool send_setting_contract_request() { settings.contract_requested_time = millis(); return send_cmd(mcSendSettingContract); }
  bool send_input_contract_request() {inputs.contract_requested_time = millis(); return send_cmd(mcSendInputContract); }
  bool send_output_contract_request() { outputs.contract_requested_time = millis(); return send_cmd(mcSendOutputContract); }
  bool send_settings_request() { settings.requested_time = millis(); return send_cmd(mcSendSettings); }
  bool send_inputs_request() { inputs.requested_time = millis(); return send_cmd(mcSendInputs); }
  bool send_outputs_request() { outputs.requested_time = millis();  return send_cmd(mcSendOutputs); }
  bool send_status_request() { status_requested_time = millis(); return send_cmd(mcSendStatus); }
  #endif // IS_MASTER
  
  bool send(uint8_t remote_id, const uint8_t *remote_bus, const uint8_t *message, uint16_t length) {
    #if defined(DEBUG_MSG) || defined(DEBUG_PRINT)
      Serial.print("Sending len "); Serial.print(length); Serial.print(" cmd "); Serial.println(message[0]);
    #endif  

    uint16_t status = pjon->send_packet(remote_id, remote_bus, (const char*)message, length, 
      is_active() ? MI_SEND_TIMEOUT : MI_REDUCED_SEND_TIMEOUT, pjon->get_header() | MI_PJON_BIT | EXTEND_HEADER_BIT);
      
    #ifdef DEBUG_PRINT
       if (status != ACK) Serial.println(F("----> Failed sending."));
    #endif    
    #ifdef IS_MASTER
      if (status == ACK) {
        last_alive = millis();
        comm_failures = 0; 
      } else if (comm_failures < 255) comm_failures++;
    #endif
    return status == ACK;
  }

  bool handle_request_message(const uint8_t *payload, const uint8_t length) {
    BinaryBuffer response;
    uint8_t response_length = 0;
    if (ModuleInterface::handle_request_message(payload, length, response, response_length)) {
      if (response.is_empty()) {
        #ifdef DEBUG_PRINT
        Serial.print(F("Out of memory replying to cmd ")); Serial.println(payload[0]);
        #endif            
        return false;
      }
      // Send an unbuffered reply
      return send(pjon->get_last_packet_info().sender_id, pjon->get_last_packet_info().sender_bus_id, response.get(), response_length);
    }
    return false;
  }
  
  bool handle_message(const uint8_t *payload, const uint16_t length, const PacketInfo &packet_info) {
    // Handle only packets marked with the MI bit
    if (!((packet_info.header & EXTEND_HEADER_BIT) && (packet_info.header & MI_PJON_BIT))) return false; // Message not meant for ModuleInterface use
    
    #if defined(DEBUG_MSG) || defined(DEBUG_PRINT)
      Serial.print(F("Received len ")); Serial.print(length); Serial.print(F(" cmd ")); Serial.println(payload[0]);   
    #endif  
    if (handle_input_message(payload, (uint8_t) length)) {
      #ifdef IS_MASTER
      last_alive = millis();
      comm_failures = 0;
      #endif
      return true;
    }
    if (handle_request_message(payload, (uint8_t) length)) {
      #ifdef IS_MASTER
      last_alive = millis(); 
      comm_failures = 0;
      #endif
      return true;
    }   
    return false;
  }  

  #ifndef IS_MASTER  
  // Make sure that the user does not have to register a receiver callback function for things to work
  static void default_receiver_function(uint8_t *payload, uint16_t length, const PacketInfo &packet_info) {
    get_singleton()->handle_message(payload, length, packet_info);
  }
  #endif  
};

#ifndef IS_MASTER
PJONModuleInterface *PJONModuleInterface::singleton = NULL;
#endif