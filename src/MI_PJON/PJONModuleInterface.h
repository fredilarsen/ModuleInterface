#pragma once

#include <MI/ModuleInterface.h>

#ifndef PJON_MAX_PACKETS
  #define PJON_MAX_PACKETS 0
#endif
#ifndef PJON_PACKET_MAX_LENGTH
  #define PJON_PACKET_MAX_LENGTH 250
#endif
// Increase SWBB timeout to handle long packets
#ifndef SWBB_RESPONSE_TIMEOUT
  #define SWBB_RESPONSE_TIMEOUT 2000
#endif

#include <MI_PJON/Link.h>

// A timeout to make sure a lost request or reply does not stop everything permanently
#define MI_REQUEST_TIMEOUT 5000000    // (us) How long to wait for an active module to reply to a request
#define MI_SEND_TIMEOUT 5000000       // (us) How long to wait for an active device to PJON_ACK
#define MI_REDUCED_SEND_TIMEOUT 10000 // (us) How long to try to contact a module that is marked as inactive

// The well-known PJON port number for ModuleInterface packets, used to quickly separate ModuleInterface related messages from others
#define MI_PJON_MODULE_INTERFACE_PORT 100

class PJONModuleInterface : public ModuleInterface {
friend class PJONModuleInterfaceSet;
protected:
  Link *pjon = NULL;
  #ifdef IS_MASTER
  uint8_t remote_id = 0;
  uint8_t remote_bus_id[4];
  uint32_t status_requested_time = 0;
  #else
  void set_link(Link &pjon) { this->pjon = &pjon; pjon.set_receiver(default_receiver_function, this); }
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
  #ifdef MI_NO_DYNAMIC_MEM
  PJONModuleInterface(const char *module_name, Link &pjon,
    const uint8_t num_settings, ModuleVariable *setting_variables, const char *settingnames, // This string must be PROGMEM
    const uint8_t num_inputs,   ModuleVariable *input_variables,   const char *inputnames,   // This string must be PROGMEM
    const uint8_t num_outputs,  ModuleVariable *output_variables,  const char *outputnames)  // This string must be PROGMEM
  {
    set_variables(num_settings, setting_variables,
                  num_inputs,   input_variables,
                  num_outputs,  output_variables);
    set_contracts_P(module_name, settingnames, inputnames, outputnames);
    set_link(pjon);
  }
  PJONModuleInterface(const char *module_name, Link &pjon,
    const uint8_t num_settings, ModuleVariable *setting_variables, MVS_getContractChar settingnames,
    const uint8_t num_inputs,   ModuleVariable *input_variables,   MVS_getContractChar inputnames,
    const uint8_t num_outputs,  ModuleVariable *output_variables,  MVS_getContractChar outputnames) {
    set_variables(num_settings, setting_variables,
                  num_inputs,   input_variables,
                  num_outputs,  output_variables);
    set_contracts(module_name, settingnames, inputnames, outputnames);
    set_link(pjon);
  }
  #else
  PJONModuleInterface(const char *module_name, Link &pjon,
                      const char *settingnames, const char *inputnames, const char *outputnames) :
    ModuleInterface(module_name, settingnames, inputnames, outputnames) {
    set_link(pjon);
  }
  PJONModuleInterface(const char *module_name, Link &pjon,
                      bool use_progmem, // Required to be true, just used to distinguish this function from the standard
                      const char * settingnames, const char *inputnames, const char *outputnames) :  // These strings must be PROGMEM
    ModuleInterface(module_name, use_progmem, settingnames, inputnames, outputnames) {
    set_link(pjon);
  }
  PJONModuleInterface(const char *module_name, Link &pjon,
                      MVS_getContractChar settingnames, MVS_getContractChar inputnames, MVS_getContractChar outputnames) :
    ModuleInterface(module_name, settingnames, inputnames, outputnames) {
    set_link(pjon);
  }
  // This constructor is available only because the F() macro can only be used in a function.
  // So if RAM is tight, use this constructor and call set_contracts from setup()
  PJONModuleInterface(Link &pjon, const uint8_t num_settings, const uint8_t num_inputs, const uint8_t num_outputs) :
    ModuleInterface(num_settings, num_inputs, num_outputs) {
    set_link(pjon);
  }
  #endif
  PJONModuleInterface(Link &pjon) : ModuleInterface() { set_link(pjon); }

  void update() {
    // Listen for packets for 1ms
    pjon->receive(1000);

    // Module-initiated event support
    send_output_events();

    // If user sketch is posting packets to be sent, send them
    pjon->update();
  }
  #endif

  void init() {
    ModuleInterface::init();
    #ifdef IS_MASTER
    memset(remote_bus_id, 0, 4);
    #endif
  }

  void set_bus(Link &bus) { pjon = &bus; }

  #ifdef IS_MASTER
  void set_remote_device(Link &pjon, uint8_t remote_id, uint8_t remote_bus_id[]) {
    this->pjon = &pjon; this->remote_id = remote_id; memcpy(this->remote_bus_id, remote_bus_id, 4);
  }

  void set_name_prefix_and_address(const char *name_and_address) { // Format like "device1:d1:44" or "device1:d1:44:0.0.0.1"
    // Split input string into name, device id and bus id
    const char *end = strchr(name_and_address, ' ');
    if (!end) end = name_and_address + strlen(name_and_address);
    uint8_t len = end - name_and_address;
    char *buf = new char[len + 1];
    if (buf == NULL) { ModuleVariableSet::out_of_memory = true; return; }
    memcpy(buf, name_and_address, len); buf[len] = 0;
    char *p1 = strchr(buf + 1, ':');
    if (p1) { *p1 = 0; p1++; } // p1 now pointing to prefix
    char *p2 = p1 != NULL ? strchr(p1 + 1, ':') : NULL;
    if (p2) { *p2 = 0; p2++; } // p2 now pointing to device id
    char *p3 = p2 != NULL ? strchr(p2 + 1, ':') : NULL;
    if (p3) { *p3 = 0; p3++; } // p3 now pointing to bus id
    // Set the values
    set_name(buf);
    if (p1) set_prefix(p1);
    if (p2) remote_id = atoi(p2);
    if (p3) {
      for (uint8_t i=0; i<4; i++) {
        char *p4 = strchr(p3, '.');
        if (p4) *p4 = 0; // Null-terminate this part of the bus id
        remote_bus_id[i] = atoi(p3);
        if (p4) p3 = p4; else break;
        p3++;
      }
    }
    delete buf;
  }

   // This function will wait up to the given timeout for a packet of the given type to arrive.
   // Other packet types may be received and handled but will not cause this function to return.
  uint16_t receive_packet(uint32_t timeout, ModuleCommand cmd) {
    uint32_t start = micros();
    uint16_t status = PJON_FAIL;
    last_incoming_cmd = mcUnknownCommand;
    do {
      status = pjon->receive();
      if ((status == PJON_ACK || status == PJON_NAK) && cmd == last_incoming_cmd) break;
#ifdef WIN32
      delay(1);
#endif
    } while (timeout > (uint32_t)(micros()-start));
    return status;
  }

  void update_contract(const uint32_t interval_ms) {
    if (!settings.got_contract() && (settings.contract_requested_time == 0 || ((uint32_t)(millis()-settings.contract_requested_time) >= interval_ms))) {
      if (send_setting_contract_request()) receive_packet(MI_REQUEST_TIMEOUT, mcSetSettingContract); else pjon->receive();
    }
    if (!inputs.got_contract() && (inputs.contract_requested_time == 0 || ((uint32_t)(millis()-inputs.contract_requested_time) >= interval_ms))) {
      if (send_input_contract_request()) receive_packet(MI_REQUEST_TIMEOUT, mcSetInputContract);  else pjon->receive();
    }
    if (!outputs.got_contract() && (outputs.contract_requested_time == 0 || ((uint32_t)(millis()-outputs.contract_requested_time) >= interval_ms))) {
      if (send_output_contract_request()) receive_packet(MI_REQUEST_TIMEOUT, mcSetOutputContract);  else pjon->receive();
    }
  }

  void update_values(const uint32_t interval_ms) {
    if (outputs.got_contract() && outputs.get_num_variables() != 0 &&
      (outputs.requested_time == 0 || ((uint32_t)(millis()-outputs.requested_time) >= interval_ms))) {
      if (send_outputs_request()) receive_packet(MI_REQUEST_TIMEOUT, mcSetOutputs); else pjon->receive();
    }
  }

  void update_settings(const uint32_t interval_ms) {
    if (((status_bits & MODIFIED_SETTINGS) != 0) && settings.got_contract() && settings.get_num_variables() != 0 &&
      (settings.requested_time == 0 || ((uint32_t)(millis()-settings.requested_time) >= interval_ms))) {
      if (send_settings_request()) receive_packet(MI_REQUEST_TIMEOUT, mcSetSettings); else pjon->receive();
    }
  }

  void update_status(const uint32_t interval_ms) {
    if (got_contract() && (status_requested_time == 0 || ((uint32_t)(millis()-status_requested_time) >= interval_ms))) {
      if (send_status_request()) receive_packet(MI_REQUEST_TIMEOUT, mcSetStatus); else pjon->receive();
    }
  }

  // Sending of data from master to remote module
  void send_settings() {
    #ifdef DEBUG_PRINT
    if (settings.got_contract() && !settings.is_updated() && settings.get_num_variables() != 0) {
      dname(); DPRINTLN(F("Settings not sent because not updated yet."));
    }
    #endif
    if (!settings.got_contract() || !settings.is_updated() || settings.get_num_variables() == 0) return;
    notify(ntSampleSettings, this);
    BinaryBuffer response;
    uint8_t response_length = 0;
    settings.get_values(response, response_length, mcSetSettings);
    send(remote_id, remote_bus_id, response.get(), response_length);
    status_bits &= ~MISSING_SETTINGS; // Assume they were received until next status saying they were not
  }

  void send_inputs() {
    #ifdef DEBUG_PRINT
    if (inputs.got_contract() && !inputs.is_updated() && inputs.get_num_variables()>0) {
      dname(); DPRINTLN(F("Inputs not sent because not updated yet."));
    }
    #endif
    if (!inputs.got_contract() || !inputs.is_updated()) return;
    notify(ntSampleInputs, this);
    BinaryBuffer response;
    uint8_t response_length = 0;
    inputs.get_values(response, response_length, mcSetInputs);
    send(remote_id, remote_bus_id, response.get(), response_length);
    status_bits &= ~MISSING_INPUTS; // Assume they were received until next status saying they were not
  }

  // Sending of requests
  bool send_cmd(const uint8_t &value) {
    #ifdef DEBUG_PRINT
    dname(); DPRINT(F("SENT REQUEST ")); DPRINTLN(value);
    #endif
    return send(remote_id, remote_bus_id, &value, 1);
  }
  bool send_request(const uint8_t &value, uint32_t &requested_time) {
    bool acked = send_cmd(value);
    requested_time = millis(); // Update time only if successfully sent
    return acked;
  }
  bool send_setting_contract_request() { return send_request(mcSendSettingContract, settings.contract_requested_time); }
  bool send_input_contract_request() { return send_request(mcSendInputContract, inputs.contract_requested_time); }
  bool send_output_contract_request() { return send_request(mcSendOutputContract, outputs.contract_requested_time); }
  bool send_settings_request() { return send_request(mcSendSettings, settings.requested_time); }
  bool send_inputs_request() { return send_request(mcSendInputs, inputs.requested_time); }
  bool send_outputs_request() { return send_request(mcSendOutputs, outputs.requested_time); }
  bool send_status_request() { return send_request(mcSendStatus, status_requested_time); }
  #endif // IS_MASTER

  bool send(uint8_t remote_id, const uint8_t *remote_bus, const uint8_t *message, uint16_t length) {
    #if defined(DEBUG_MSG) || defined(DEBUG_PRINT)
    dname(); DPRINT("S "); DPRINT(remote_id); DPRINT(" bus ");
    DPRINT(remote_bus[0]); DPRINT("."); DPRINT(remote_bus[1]); DPRINT(".");
    DPRINT(remote_bus[2]); DPRINT("."); DPRINT(remote_bus[3]);  
    DPRINT(" len "); DPRINT(length);
    DPRINT(" cmd "); DPRINT(message[0]); DPRINT(" active "); DPRINTLN(is_active());
    #endif
    uint16_t status = pjon->send_packet(remote_id, remote_bus, (const char*)message, length,
      is_active() ? MI_SEND_TIMEOUT : MI_REDUCED_SEND_TIMEOUT);

    #ifdef DEBUG_PRINT
    if (status != PJON_ACK) { dname(); DPRINTLN(F("----> Failed sending.")); }
    #endif
    #ifdef IS_MASTER
    if (status == PJON_ACK) {
      //last_alive = millis();  // Disabled so that reply from extender does not flag module as alive
      // comm_failures = 0; // Disabled so that reply from extender does not flag module as alive
    } else if (comm_failures < 255) comm_failures++;
    #endif

    return status == PJON_ACK;
  }

  bool handle_request_message(const uint8_t *payload, const uint8_t length) {
    BinaryBuffer response;
    uint8_t response_length = 0;
    if (ModuleInterface::handle_request_message(payload, length, response, response_length)) {
      if (response.is_empty()) {
        #ifdef DEBUG_PRINT
        dname(); DPRINT(F("Out of memory replying to cmd ")); DPRINTLN(payload[0]);
        #endif
        return false;
      }
      // Send an unbuffered reply
      return send(pjon->get_last_packet_info().sender_id, pjon->get_last_packet_info().sender_bus_id, response.get(), response_length);
    }
    return false;
  }

  bool handle_message(const uint8_t *payload, const uint16_t length, const PJON_Packet_Info &packet_info) {
    // Handle only packets marked with the MI port
    if (!((packet_info.header & PJON_PORT_BIT) &&
      (packet_info.port == MI_PJON_MODULE_INTERFACE_PORT))) return false; // Message not meant for ModuleInterface use

    #if defined(DEBUG_MSG) || defined(DEBUG_PRINT)
    dname(); DPRINT(F("R len ")); DPRINT(length); DPRINT(F(" cmd ")); DPRINTLN(payload[0]);
    #endif
    if (handle_input_message(payload, (uint8_t) length)) {
      #ifdef IS_MASTER
      last_alive = millis();
      comm_failures = 0;
      if (length > 0) last_incoming_cmd = (ModuleCommand) payload[0];
      #endif
      return true;
    }
    if (handle_request_message(payload, (uint8_t) length)) {
      #ifdef IS_MASTER
      last_alive = millis();
      comm_failures = 0;
      if (length > 0) last_incoming_cmd = (ModuleCommand) payload[0];
      #endif
      return true;
    }
    return false;
  }

  #ifdef IS_MASTER
  // If any input is flagged as an event, send it immediately to the module from master
  void send_input_events() {
    BinaryBuffer response;
    uint8_t response_length;
    inputs.get_values(response, response_length, mcSetInputs, true);
    if (response_length > 0) {
      #ifdef DEBUG_PRINT
      dname(); DPRINT("send_input_events, length "); DPRINT(response_length); DPRINT(", module id ");
      DPRINTLN(remote_id);
      #endif
      if (send(remote_id, remote_bus_id, response.get(), response_length)) inputs.clear_events();
    }
  }
  #endif

  #ifndef IS_MASTER
  // If any output is flagged as an event, send it immediately to the master (breaking the master-slave pattern)
  void send_output_events() {
    BinaryBuffer response;
    uint8_t response_length;
    outputs.get_values(response, response_length, mcSetOutputs, true);
    // TODO: Do not assume that the latest packet is from master!
    if (response_length > 0 && pjon->get_last_packet_info().sender_id != PJON_NOT_ASSIGNED && pjon->get_last_packet_info().sender_id != 0) {
      #ifdef DEBUG_PRINT
      dname(); DPRINT("send_output_events, length "); DPRINT(response_length); DPRINT(", master id ");
      DPRINTLN(pjon->get_last_packet_info().sender_id);
      #endif
      if (send(pjon->get_last_packet_info().sender_id, pjon->get_last_packet_info().sender_bus_id, response.get(), response_length))
        outputs.clear_events();
    }
  }

  // Make sure that the user does not have to register a receiver callback function for things to work
  static void default_receiver_function(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info) {
    if (packet_info.custom_pointer)
      ((PJONModuleInterface*) packet_info.custom_pointer)->handle_message(payload, length, packet_info);
  }
  #endif
};
