#pragma once

// This define helps to reduce module code size by including master-specific code only when master
#define IS_MASTER      // Can be defined manually if using ModuleInterface directly on master side for one-to-one
#define USE_MIVARIABLE // Can be defined manually to use this on module side as well

// Value 255 is reserved to mean "no module"
#define NO_MODULE 255

#include <ModuleInterface.h>

class ModuleInterfaceSet {
protected:
  char moduleset_prefix[MVAR_PREFIX_LENGTH+1]; // A unique lower case prefix, usefule if there are multiple masters connected to same db
  bool updated_intermodule_dependencies = false;  
public:
  uint8_t num_interfaces = 0;
  ModuleInterface **interfaces = NULL;
  
  ModuleInterfaceSet(const char *prefix = NULL) { set_prefix(prefix); }  
  ModuleInterfaceSet(const uint8_t num_interfaces, const char *prefix = NULL) {
    set_prefix(prefix);
    this->num_interfaces = num_interfaces; interfaces = new ModuleInterface*[num_interfaces];
    for (uint8_t i=0; i < num_interfaces; i++) {
      interfaces[i] = new ModuleInterface();
      if (interfaces[i] == NULL) {
        ModuleVariableSet::out_of_memory = true;
        #ifdef DEBUG_PRINT
        Serial.println(F("MIS::constr OUT OF MEMORY"));
        #endif
      }
    }
  }
  ~ModuleInterfaceSet() {
    if (interfaces != NULL) {
      for (int i=0; i < num_interfaces; i++) delete interfaces[i];
      delete interfaces;
    }
  }
  void set_prefix(const char *prefix) {
    if (prefix == NULL) moduleset_prefix[0] = 0; 
    else {
      strncpy(moduleset_prefix, prefix, MVAR_PREFIX_LENGTH); 
      moduleset_prefix[MVAR_PREFIX_LENGTH] = 0;
    }
  }
  const char *get_prefix() const { return moduleset_prefix; }
  
  void assign_names(const char *names[]) { for (int i=0; i<num_interfaces; i++) interfaces[i]->set_name(names[i]); }
  ModuleInterface *operator [] (const uint8_t ix) { return (interfaces[ix]); }
  
  void update_intermodule_dependencies() {
    if (!updated_intermodule_dependencies && got_all_contracts()) {
      for (uint8_t i = 0; i < num_interfaces; i++) {
        interfaces[i]->allocate_source_arrays();
        for (int j=0; j<interfaces[i]->inputs.get_num_variables(); j++) {
          find_output_by_name(interfaces[i]->inputs.get_module_variable(j).name,
                              interfaces[i]->input_source_module_ix[j],
                              interfaces[i]->input_source_output_ix[j]);
        }
      }
      updated_intermodule_dependencies = true;
    }
  }
  
  // If there are inter-module dependencies, then transfer values between modules (outputs to inputs)
  void transfer_outputs_to_inputs() {
    if (!got_all_contracts()) {
      #ifdef DEBUG_PRINT
      Serial.println(F("** ALL CONTRACTS NOT SET"));
      #endif
      return;
    }
    if (!updated_intermodule_dependencies) update_intermodule_dependencies();
    uint8_t buf[4]; // Largest value possibly encountered
    for (int i=0; i<num_interfaces; i++) {
      bool all_sources_updated = true, some_set = false;
      for (int j=0; j<interfaces[i]->inputs.get_num_variables(); j++) {
        uint8_t module_ix = interfaces[i]->input_source_module_ix[j], var_ix = interfaces[i]->input_source_output_ix[j];
        if (module_ix != NO_MODULE && var_ix != NO_VARIABLE && interfaces[module_ix]->outputs.is_updated()) {     
          uint8_t size = interfaces[i]->inputs.get_module_variable(j).get_size();
          memset(buf, 0, 4);
          interfaces[module_ix]->outputs.get_value(var_ix, buf, size);
          interfaces[i]->inputs.set_value(j, buf, size);
          if (!interfaces[module_ix]->outputs.is_updated()) all_sources_updated = false; // Source not updated yet
          some_set = true;
        }
      }
      // Flag inputs as ready for transfer
      if (some_set) {
        if (all_sources_updated) interfaces[i]->inputs.set_updated();
        #ifdef DEBUG_PRINT
        else {
          static uint32_t last = 0;
          if (millis() - last >= 100) {
            last = millis();            
            Serial.print(F("Inputs NOT UPDATED for interface ")); Serial.print(i); Serial.println(F(" because of old source outputs."));
          }
        }
        #endif
      }
    }
  }
  
  // If any outputs have been flagged as events, also set value and the event flag on inputs that are using them.
  void transfer_events_from_outputs_to_inputs() {
    uint8_t buf[4]; // Largest value possibly encountered
    for (uint8_t i = 0; i < num_interfaces; i++) {
      for (uint8_t j = 0; j < interfaces[i]->inputs.get_num_variables(); j++) {
        uint8_t module_ix = interfaces[i]->input_source_module_ix[j], var_ix = interfaces[i]->input_source_output_ix[j];
        if (module_ix != NO_MODULE && var_ix != NO_VARIABLE) {
          if (interfaces[module_ix]->outputs.get_module_variable(var_ix).is_event()) {
            // Copy value
            uint8_t size = interfaces[i]->inputs.get_module_variable(j).get_size();
            memset(buf, 0, 4);
            interfaces[module_ix]->outputs.get_value(var_ix, buf, size);
            interfaces[i]->inputs.set_value(j, buf, size);
            // Set event flag
            interfaces[i]->inputs.set_event(j);
          }
        }
      }
    }    
  }
  
  bool got_all_contracts() {
    for (uint8_t i = 0; i < num_interfaces; i++) {
      if (!interfaces[i]->got_contract() && interfaces[i]->is_active()) {
        updated_intermodule_dependencies = false; // Interconnections no longer valid, must be recomputed
        return false;
      }
    }
    return true;
  }
  
  // If a device gets unplugged or dies, it will register as inactive after a while.
  // Get the count of inactive modules
  uint8_t get_inactive_module_count() {
    uint8_t cnt = 0;
    for (uint8_t i=0; i<num_interfaces; i++) if (!interfaces[i]->is_active()) cnt++;
    return cnt;
  }  
  
  // Register notification callback function common for all interfaces.
  // (Can register individual callback functions instead by calling the associated ModuleInterface function directly.)
  void set_notification_callback(notify_function n) { 
    for (uint8_t i = 0; i < num_interfaces; i++) interfaces[i]->set_notification_callback(n);
  }
  
  bool find_output_by_name(const char *name, uint8_t &module_ix, uint8_t &output_ix) const {
    char prefixed_name[MVAR_MAX_NAME_LENGTH + MVAR_PREFIX_LENGTH + 1];
    for (uint8_t i=0; i<num_interfaces; i++) {
      for (uint8_t j=0; j < interfaces[i]->outputs.get_num_variables(); j++) {
        interfaces[i]->outputs.get_module_variable(j).get_prefixed_name(interfaces[i]->get_prefix(), prefixed_name, sizeof prefixed_name);
        if (strcmp(name, prefixed_name) == 0) {
          module_ix = i;
          output_ix = j;
          return true;
        }
      }
    }
    module_ix = output_ix = NO_MODULE;
    return false;
  }  
};
