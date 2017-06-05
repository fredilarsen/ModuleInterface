#pragma once

#include <MI/ModuleInterfaceSet.h>
#include <MI_PJON/PJONModuleInterface.h>

typedef void (*mis_receive_function)(const uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info, const ModuleInterface *module_interface);

void mis_global_receive_function(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info);

class PJONModuleInterfaceSet : public ModuleInterfaceSet {
protected:  
  uint32_t last_time_sync = 0;
  Link *pjon = NULL;
  static PJONModuleInterfaceSet *singleton;
  mis_receive_function custom_receive_function = NULL;
  friend void mis_global_receive_function(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info);  
public:  
  PJONModuleInterfaceSet(const char *prefix = NULL) : ModuleInterfaceSet(prefix) { init(); }
  PJONModuleInterfaceSet(Link &bus, const uint8_t num_interfaces, const char *prefix = NULL) : ModuleInterfaceSet(prefix) {
    init(); this->num_interfaces = num_interfaces; interfaces = new ModuleInterface*[num_interfaces];
    for (uint8_t i = 0; i < num_interfaces; i++) interfaces[i] = new PJONModuleInterface();
    pjon = &bus;
    pjon->set_receiver(mis_global_receive_function);
  }
  // Specifying modules as textual list like "BlinkModule:bm:44 TestModule:tm:44:0.0.0.1":
  PJONModuleInterfaceSet(Link &bus, const char *interface_list, const char *prefix = NULL) : ModuleInterfaceSet(prefix) { 
    // Count number of interfaces
    num_interfaces = 0;
    const char *p = interface_list;
    while (*p != 0) { p++; if (*p == 0 || *p == ' ') num_interfaces++; }
    
    // Allocate
    init(); interfaces = new ModuleInterface*[num_interfaces];
    for (uint8_t i = 0; i < num_interfaces; i++) interfaces[i] = new PJONModuleInterface();
    
    // Set names and ids
    p = interface_list;
    uint8_t cnt = 0;
    while (*p != 0) {
      ((PJONModuleInterface*) (interfaces[cnt]))->set_name_prefix_and_address(p);
      ((PJONModuleInterface*) (interfaces[cnt]))->set_bus(bus);
      cnt++;
      while (*p != 0 && *p != ' ') p++;
      if (*p == ' ') p++; // First char after space
    }
    pjon = &bus;
    pjon->set_receiver(mis_global_receive_function);
  }
  void init() { singleton = this; }
  
  static PJONModuleInterfaceSet *get_singleton() { return singleton; }
  
  void set_receiver(mis_receive_function r) {
    pjon->set_receiver(mis_global_receive_function); // Make sure main receiver function is registered
    custom_receive_function = r; // Register custom/user callback function to receive non-ModuleInterface related messages
  }
    
  void update_contracts() { for (uint8_t i = 0; i < num_interfaces; i++) ((PJONModuleInterface*) (interfaces[i]))->update_contract(sampling_time_outputs); }
  void update_values() { for (uint8_t i = 0; i < num_interfaces; i++) ((PJONModuleInterface*) (interfaces[i]))->update_values(sampling_time_outputs); }
  void update_statuses() { for (uint8_t i = 0; i < num_interfaces; i++) ((PJONModuleInterface*) (interfaces[i]))->update_status(sampling_time_outputs); }
  void send_settings() { for (uint8_t i = 0; i < num_interfaces; i++) ((PJONModuleInterface*) (interfaces[i]))->send_settings(); }
  void send_inputs() { for (uint8_t i = 0; i < num_interfaces; i++) ((PJONModuleInterface*) (interfaces[i]))->send_inputs(); }
  void send_input_events() { for (uint8_t i = 0; i < num_interfaces; i++) ((PJONModuleInterface*) (interfaces[i]))->send_input_events(); }

  void clear_output_events() { for (uint8_t i = 0; i < num_interfaces; i++) ((PJONModuleInterface*) (interfaces[i]))->outputs.clear_events(); }
  void clear_input_events() { for (uint8_t i = 0; i < num_interfaces; i++) ((PJONModuleInterface*) (interfaces[i]))->inputs.clear_events(); }

  void handle_events() {
    if (!updated_intermodule_dependencies || !got_all_contracts()) return;
    transfer_events_from_outputs_to_inputs();
    send_input_events();
    clear_output_events();
    clear_input_events();
  }
  
  // Time will be broadcast to all modules unless NO_TIME_SYNC is defined or master itself is not timesynced
  #ifndef NO_TIME_SYNC
  void broadcast_time() {
    if (miIsTimeSynced()) {
      bool do_sync = ((uint32_t)(millis() - last_time_sync) >= 60000); // Time for a scheduled broadcast?
      #ifdef DEBUG_PRINT
        if (do_sync) {Serial.print(F("Scheduled broadcast of time sync: ")); Serial.println(miGetTime()); }
      #endif
      // Check if any module has reported that is it missing time
      if (!do_sync) {
        for (uint8_t i = 0; i < num_interfaces; i++) if (interfaces[i]->status_bits & MISSING_TIME) {
          #ifdef DEBUG_PRINT
            Serial.print(F("Module ")); Serial.print(interfaces[i]->module_name);
            Serial.print(F(" missing time, broadcasting: ")); Serial.println(miGetTime());
          #endif
          do_sync = true;
          break;
        }
      }
      if (do_sync) {
        last_time_sync = millis();
        char buf[5];
        buf[0] = (char) mcSetTime;
        uint32_t t = miGetTime();
        memcpy(&buf[1], &t, 4);
        uint32_t dummy_bus_id = 0;
        uint16_t packet = pjon->send_packet(0, (uint8_t*)&dummy_bus_id, buf, 5, MI_REDUCED_SEND_TIMEOUT, 
          pjon->get_header() | PJON_EXT_HEAD_BIT | MI_PJON_BIT);
        
        // Clear time-missing bit to avoid this triggering continuous broadcasts.
        // If a module did not pick up the broadcast, we will get this information in the next status reply.
        for (uint8_t i = 0; i < num_interfaces; i++) interfaces[i]->status_bits &= ~MISSING_TIME;
      }
    }
  }
  #endif
  
  void update() {
    // Do PJON send and receive
    pjon->update();
    pjon->receive(100);
    
    // Handle incoming events
    handle_events();
    
    // Broadcast time to all modules with a few minutes interval
    #ifndef NO_TIME_SYNC
    broadcast_time();
    #endif
    
    // Request the contract of each module if not received already
    update_contracts();  
  
    // Send settings to each module
    if (millis() - last_settings_sent >= sampling_time_settings) {
      last_settings_sent = millis();
      send_settings();
    }
    
    // Get fresh output values from each module
    update_values();

    // Deliver inputs to each module
    if (millis() - last_inputs_sent >= sampling_time_outputs) {
      last_inputs_sent = millis();
      // Transfer outputs from modules to inputs of other modules
      transfer_outputs_to_inputs();
      
      send_inputs();
    }

    // Get fresh status from each module
    update_statuses();
  }
  
  uint8_t locate_module(const uint8_t device_id, const uint8_t *bus_id) const {
    int8_t ix = NO_MODULE;
    for (uint8_t i=0; i<num_interfaces; i++) {
      if (((PJONModuleInterface*) interfaces[i])->remote_id == device_id && 
          (memcmp(((PJONModuleInterface*) interfaces[i])->remote_bus_id, bus_id, 4) == 0)) {
        ix = i; break;
      }
    }
    return ix;  
  }
  
  bool handle_message(const uint8_t *payload, const uint16_t length, const PJON_Packet_Info &packet_info) {
    // Locate the relevant module based on packet info (device id and bus id)
    int8_t ix = locate_module(packet_info.sender_id, packet_info.sender_bus_id);
    if (ix == NO_MODULE) return false;
    
    // Let the interface handle the message
    return ((PJONModuleInterface*) interfaces[ix])->handle_message(payload, length, packet_info);
  }
  
  // These settings specify how often to transfer settings and outputs 
  uint16_t sampling_time_settings = 10000,
           sampling_time_outputs = 10000;
  uint32_t last_settings_sent = 0,
           last_inputs_sent = 0;  
};

PJONModuleInterfaceSet *PJONModuleInterfaceSet::singleton = NULL;

// PJON receive callback function
void mis_global_receive_function(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info) {
  PJONModuleInterfaceSet *singleton = PJONModuleInterfaceSet::get_singleton();
  if (singleton) {
    // Find out which module is sending
    uint8_t ix = singleton->locate_module(packet_info.sender_id, packet_info.sender_bus_id);
    PJONModuleInterface *interface = NULL;
    if (ix != NO_MODULE) {  
      interface = (PJONModuleInterface*) singleton->interfaces[ix];
      
      // Let interface handle ModuleInterface related message
      if (interface->handle_message(payload, length, packet_info)) return;
    }
    // If we get here, the message was not a recognized ModuleInterface related message. Pass it to the user-defined callback.
    if (singleton->custom_receive_function) singleton->custom_receive_function(payload, length, packet_info, interface);    
  }
}

