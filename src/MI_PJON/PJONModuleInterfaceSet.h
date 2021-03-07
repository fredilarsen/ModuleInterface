#pragma once

#include <MI/ModuleInterfaceSet.h>
#include <MI/MITransferBase.h>
#include <MI_PJON/PJONModuleInterface.h>

#if defined(LINUX) || defined(ANDROID) || defined(_WIN32)
  #define MI_ALLOW_MODULELIST_CHANGES
#endif 

typedef void (*mis_receive_function)(const uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info, const ModuleInterface *module_interface);

void mis_global_receive_function(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info);

class PJONModuleInterfaceSet : public ModuleInterfaceSet {
protected:
  uint32_t last_time_sync = 0;
  MILink *pjon = NULL;
  mis_receive_function custom_receive_function = NULL;
  friend void mis_global_receive_function(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info);
  #ifdef MI_ALLOW_MODULELIST_CHANGES
  // Remember the list of modules and allow it to change
  char *module_list = NULL;
  #endif
  // External systems to transfer to/from
  uint8_t external_count = 0;
  MITransferBase **external_transfer = NULL;

    // These settings specify how often to transfer settings, outputs and inputs
  uint16_t sampling_time = 10000;
  uint32_t last_sampled = 0;
public:
  PJONModuleInterfaceSet(const char *prefix = NULL) : ModuleInterfaceSet(prefix) { init(); }
  PJONModuleInterfaceSet(MILink &bus, const uint8_t num_interfaces, const char *prefix = NULL) : ModuleInterfaceSet(prefix) {
    init(); 
    this->num_interfaces = num_interfaces; 
    if (num_interfaces > 0) {
      interfaces = new ModuleInterface*[num_interfaces];
      for (uint8_t i = 0; i < num_interfaces; i++) interfaces[i] = new PJONModuleInterface();
    }
    pjon = &bus;
    pjon->set_receiver(mis_global_receive_function, this);
  }
  // Specifying modules as textual list like "BlinkModule:bm:44 TestModule:tm:44:0.0.0.1":
  PJONModuleInterfaceSet(MILink &bus, const char *interface_list, const char *prefix = NULL) : ModuleInterfaceSet(prefix) {
    init();
    pjon = &bus;
    pjon->set_receiver(mis_global_receive_function, this);
    if (interface_list) set_interface_list(interface_list);
  }
  ~PJONModuleInterfaceSet() {
    #ifdef MI_ALLOW_MODULELIST_CHANGES
    if (module_list != NULL) delete module_list;
    #endif
  }
  void init() { }
  void set_interface_list(const char *interface_list) {
    #ifdef MI_ALLOW_MODULELIST_CHANGES
    if (interface_list == NULL || strlen(interface_list) == 0) return;
    if (num_interfaces > 0) {
      // Just return if there are no changes
      if (module_list != NULL && strcmp(module_list, interface_list)==0) return; 

      // Clear all existing setup
      for (uint8_t i = 0; i < num_interfaces; i++) {
        if (interfaces[i] != NULL) delete interfaces[i];
      }
      delete[] interfaces;
      num_interfaces = 0;
      last_time_sync = 0;
      updated_intermodule_dependencies = false;
    }

    // Remember module list
    if (module_list != NULL) delete module_list;
    module_list = strdup(interface_list);

    #else
    // This function can only be called once after startup
    if (num_interfaces > 0) return;
    #endif

    // Count number of interfaces
    const char *p = interface_list;
    while (*p != 0) {
      p++; 
      if (*p == 0 || *p == ' ') num_interfaces++;
      while (*p == ' ') p++; // Allow multiple spaces in sequence
    }

    // Allocate
    interfaces = new ModuleInterface*[num_interfaces];
    for (uint8_t i = 0; i < num_interfaces; i++) interfaces[i] = new PJONModuleInterface();

    // Set names and ids
    p = interface_list;
    uint8_t cnt = 0;
    #ifdef DEBUG_PRINT
    DPRINT("Interface count="); DPRINT(num_interfaces); DPRINT(": ");
    #endif
    while (*p != 0 && cnt < num_interfaces) {
      ((PJONModuleInterface*) (interfaces[cnt]))->set_name_prefix_and_address(p);
      ((PJONModuleInterface*) (interfaces[cnt]))->set_bus(*pjon);
      #ifdef DEBUG_PRINT
      if (cnt>0) DPRINT(", "); DPRINT(((PJONModuleInterface*) (interfaces[cnt]))->module_name);
      #endif
      cnt++;
      while (*p != 0 && *p != ' ') p++;
      while (*p == ' ') p++; // Find char after spaces
    }
    #ifdef DEBUG_PRINT
    DPRINTLN("");
    #endif
  }
  MILink *get_link() { return pjon; }

  void set_receiver(mis_receive_function r) {
    pjon->set_receiver(mis_global_receive_function, this); // Make sure main receiver function is registered
    custom_receive_function = r; // Register custom/user callback function to receive non-ModuleInterface related messages
  }

  // Register an array of pointers to MITransferBase objects existing on the outside.
  // We do NOT take over the memory management, they will be deallocated on the outside.
  void set_external_transfer(uint8_t count, MITransferBase *transfer[]) {
    external_count = count;
    external_transfer = transfer;
  }

  // Setting and getting the interval between transfers of settings, outputs and inputs
  void set_transfer_interval(uint32_t interval_millis) { sampling_time = interval_millis; }
  uint32_t get_transfer_interval() { return sampling_time; }

  void update_contracts() { 
    for (uint8_t i = 0; i < num_interfaces; i++) {
      ((PJONModuleInterface*) (interfaces[i]))->update_contract(interfaces[i]->is_active() ? 1000 : 20000);
      check_incoming();
    }
  }

  // This sends settings (can be empty) to each module, and receives a reponse containing 
  // outputs (can be empty) and status.
  void update_settings() { 
    for (uint8_t i = 0; i < num_interfaces; i++) {
      ((PJONModuleInterface*) (interfaces[i]))->update_settings(1); 
      check_incoming();
    }
  }

  void send_settings() { 
    for (uint8_t i = 0; i < num_interfaces; i++) {
      // Send settings, then wait for outputs
      if (((PJONModuleInterface*) interfaces[i])->send_settings())
        ((PJONModuleInterface*) interfaces[i])->receive_packet(((PJONModuleInterface*) interfaces[i])->get_request_timeout(), mcSetOutputs);
      check_incoming();
    }
  }
  void send_inputs() { 
    for (uint8_t i = 0; i < num_interfaces; i++) {
      ((PJONModuleInterface*) (interfaces[i]))->send_inputs(); 
      check_incoming();
    }
  }
  void send_input_events() { for (uint8_t i = 0; i < num_interfaces; i++) ((PJONModuleInterface*) (interfaces[i]))->send_input_events(); }
  void send_setting_events() { for (uint8_t i = 0; i < num_interfaces; i++) ((PJONModuleInterface*) (interfaces[i]))->send_setting_events(); }

  // Check for incoming packets, send events immediately if present
  void check_incoming() {
    pjon->receive();
    handle_events();
    pjon->update();
  }

  void handle_events() {
    if (!got_all_contracts()) return;

    if (updated_intermodule_dependencies) {
      // Get inputs and send them to modules
      transfer_events_from_outputs_to_inputs(); // Copy received module outputs to module inputs
      get_input_events_from_external();         // Get from external to inputs- TODO happens async when received?
//TODO check input events?    
      send_input_events();                      // Send to modules
      clear_input_events();                     // Clear after sending
    }

    // Send output and setting events to MITransferBases
    put_events_to_external();   // Send to external
    clear_output_events();      // Clear after sending

   // TODO: Check setting events, must be an event flag for each direction?
//    clear_setting_events(true); // Clear setting events from modules

    // Send settings events to modules
    send_setting_events();
    clear_setting_events(); // Clear setting events to modules
  }

  // Time will be broadcast to all modules unless NO_TIME_SYNC is defined or master itself is not timesynced
  #ifndef NO_TIME_SYNC
  void broadcast_time() {
    if (miTime::IsSynced()) {
      bool scheduled_sync = ((uint32_t)(millis() - last_time_sync) >= 60000); // Time for a scheduled broadcast?
      #ifdef DEBUG_PRINT
        if (scheduled_sync) { DPRINT(F("Scheduled broadcast of time sync: ")); DPRINTLN(miTime::Get()); }
      #endif
      
      // Check if any local bus module has reported that is it missing time
      bool broadcast = scheduled_sync;
      if (!scheduled_sync) {
        for (uint8_t i = 0; i < num_interfaces; i++) { 
          if ((interfaces[i]->status_bits & MISSING_TIME) && has_local_bus(i)) {
            // Same bus, can do broadcast
            broadcast = true;
            #ifdef DEBUG_PRINT
            if (!scheduled_sync) {
              DPRINT(F("Module ")); DPRINT(interfaces[i]->module_name);
              DPRINT(F(" missing time, broadcasting: ")); DPRINTLN(miTime::Get());
            }
            #endif
          }
        }
      }

      // Do the broadcast on local bus
      if (broadcast) {
        send_timesync(0, pjon->get_bus_id());

        // Clear time-missing bit to avoid this triggering continuous broadcasts.
        // If a module did not pick up the broadcast, we will get this information in the next status reply.
        for (uint8_t i = 0; i < num_interfaces; i++) 
          if (has_local_bus(i)) interfaces[i]->status_bits &= ~MISSING_TIME;
      }
      
      // Broadcast will not reach devices on other buses, so send directed time sync to each
      for (uint8_t i = 0; i < num_interfaces; i++) { 
        if ((scheduled_sync || interfaces[i]->status_bits & MISSING_TIME) && !has_local_bus(i)) {
          send_timesync(((PJONModuleInterface*)interfaces[i])->remote_id, ((PJONModuleInterface*)interfaces[i])->remote_bus_id);
          #ifdef DEBUG_PRINT
          if (interfaces[i]->status_bits & MISSING_TIME) DPRINT(F("Module missing time. ")); 
          DPRINT(F("Sending directed sync to "));DPRINT(interfaces[i]->module_name);
          DPRINT(F(": ")); DPRINTLN(miTime::Get());
          #endif

          // Clear time-missing bit to avoid this triggering continuous time sync to this device
          // If a module did not pick up the sync, we will get this information in the next status reply.
          interfaces[i]->status_bits &= ~MISSING_TIME;  
        }
      }
      if (scheduled_sync) last_time_sync = millis();      
    }
  }
  
  bool has_local_bus(uint8_t interface_ix) {
    // Returns true if device is on the same bus as me (the master). Always returns true in local mode.
    return (memcmp(((PJONModuleInterface*)interfaces[interface_ix])->remote_bus_id, pjon->get_bus_id(), 4)==0);
  }
  
  void send_timesync(const uint8_t id, const uint8_t bus_id[]) {
    char buf[5];
    buf[0] = (char) mcSetTime;
    uint32_t t = miTime::Get();
    int16_t offset = miTime::GetTimeZoneOffsetMinutes();
    memcpy(&buf[1], &t, 4);
    memcpy(&buf[5], &offset, 2);
    pjon->send_packet(id, bus_id, buf, 7, MI_REDUCED_SEND_TIMEOUT);
    pjon->receive(); // Just called regularly to be responsive to events
  }
  #endif
  
  // For backward compatibility
  void update(MITransferBase *transfer) {
    external_count = transfer == NULL ? 0 : 1;
    external_transfer = &transfer;
    update();
  }

  // For backward compatibility
  void update(int count, MITransferBase *transfer[]) {
    external_count = transfer == NULL ? 0 : count;
    external_transfer = transfer;
    update();
  }

  void update() {
    uint32_t start = millis();
    update_frequent();
    bool initiated = got_all_contracts();
    if (initiated && external_count && external_transfer) {
      #ifdef MASTER_MULTI_TRANSFER
      MITransferBase::transfer_count = external_count;
      #endif
      for (uint8_t t = 0; t < external_count; t++) {
        #ifdef MASTER_MULTI_TRANSFER
        external_transfer[t]->transfer_ix = t; // Associate with a changed-bit in ModuleVariables
        #endif
        external_transfer[t]->update();
      }
    }
    #ifdef DEBUG_PRINT_TIMES
    uint32_t diff = (uint32_t)(millis() - start);
    if (diff > 500) printf("Long time (%dms) in update_frequent!\n", diff);
    static uint32_t last_print = millis();
    #endif
    if (initiated && mi_interval_elapsed(last_sampled, sampling_time)) {
      #ifdef DEBUG_PRINT_TIMES
      uint32_t printdiff = (uint32_t)(millis() - last_print);
      last_print = millis();
      #endif
      start = millis();
      // Transfer settings to and from the modules, get outputs and status from the modules,
      // send to subscribing modules
      transfer_all();  // Get outputs and send to subscribing modules
      last_total_usage_ms = (uint32_t)(millis()-start);
      #ifdef DEBUG_PRINT_TIMES
      printf("Spent %dms in interval_transfer, %dms since last.\n", last_total_usage_ms, printdiff);
      #endif
    }
  }

  // This should be called as often as possible, to handle events and other prioritized tasks
  void update_frequent() {
    // Do PJON send and receive
    pjon->update();
    pjon->receive();

    // Request the contract of each module if not received already
    update_contracts();

    // Get anything coming in, and send events if any to external
    update_external();

    // Handle incoming+outgoing events
    handle_events();
  }

  void transfer_all() {
    // Send settings and get outputs and status from modules
    transfer_settings();
    update_frequent();

    // Transfer outputs from modules to inputs of other modules
    transfer_outputs_to_inputs();
    update_frequent();

    // Data exchange to web server or other system
    send_to_external();

    // Send updated inputs to all modules
    send_inputs();
    update_frequent();

    // Broadcast time to all modules with a few minutes interval
    #ifndef NO_TIME_SYNC
    broadcast_time();
    update_frequent();
    #endif
  }

  void send_to_external() {
    if (external_count && external_transfer) 
      for (uint8_t t = 0; t < external_count; t++) external_transfer[t]->put_values();
  }

  void update_external() {
    if (external_count && external_transfer) 
      for (uint8_t t = 0; t < external_count; t++) external_transfer[t]->update();
  }

//TODO:
  void get_input_events_from_external() {
    // External inputs mapped directly to module inputs will set the event flag when received.
    // But add support for the external source having a set of outputs, and route these to
    // module inputs matching the names.
    
  }

  void put_events_to_external() {
    if (external_count && external_transfer) {
      for (uint8_t t = 0; t < external_count; t++) external_transfer[t]->put_events();
/*      
      for (uint8_t i = 0; i < num_interfaces; i++) {
          interfaces[i]->outputs.clear_events();
          interfaces[i]->settings.clear_events();
      }
*/      
    }
  }

  void transfer_settings() {
    // Get potentially modified settings from each module
    update_settings();

    // Data exchange from and to web server or other system
    if (external_count && external_transfer) {
      for (uint8_t t = 0; t < external_count; t++) external_transfer[t]->put_settings();
      for (uint8_t t = 0; t < external_count; t++) external_transfer[t]->get_settings();
      #ifdef MASTER_MULTI_TRANSFER
      // Clear changed-flag if all transfer targets have received the upward change back down
      for (uint8_t i = 0; i < num_interfaces; i++) {
        for (uint8_t v = 0; v < interfaces[i]->settings.get_num_variables(); v++) {
          ModuleVariable &mv = interfaces[i]->settings.get_module_variable(v);
          if (!mv.any_change_bit(external_count)) {
            #ifdef DEBUG_PRINT_SETTINGSYNC
            if (mv.is_changed())
              printf("CLEAR CHANGED VALUE %ld cbits: %d\n", mv.get_uint32(), mv.change_bits);
            #endif
            mv.set_changed(false);
          }
        }
      }
      #endif
    }

    // Send settings to each module
    send_settings();
  }

  uint8_t locate_module(const uint8_t device_id, const uint8_t *bus_id) const {
    uint8_t ix = NO_MODULE;
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
    uint8_t ix = locate_module(packet_info.tx.id, packet_info.tx.bus_id);
    if (ix == NO_MODULE) return false;

    // Let the interface handle the message
    return ((PJONModuleInterface*) interfaces[ix])->handle_message(payload, length, packet_info);
  }
};

// PJON receive callback function
void mis_global_receive_function(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info) {
  if (packet_info.custom_pointer) {
    PJONModuleInterfaceSet *mis = (PJONModuleInterfaceSet*) packet_info.custom_pointer;

    // Find out which module is sending
    uint8_t ix = mis->locate_module(packet_info.tx.id, packet_info.tx.bus_id);
    PJONModuleInterface *interface = NULL;
    if (ix != NO_MODULE) {
      interface = (PJONModuleInterface*) mis->interfaces[ix];

      // Let interface handle ModuleInterface related message
      if (interface->handle_message(payload, length, packet_info)) return;
    }
    // If we get here, the message was not a recognized ModuleInterface related message. Pass it to the user-defined callback.
    if (mis->custom_receive_function) mis->custom_receive_function(payload, length, packet_info, interface);
  }
}
