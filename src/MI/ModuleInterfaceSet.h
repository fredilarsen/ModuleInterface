#pragma once

// This define helps to reduce module code size by including master-specific code only when master
#define IS_MASTER      // Can be defined manually if using ModuleInterface directly on master side for one-to-one
#define USE_MIVARIABLE // Can be defined manually to use this on module side as well

// Value 255 is reserved to mean "no module"
#define NO_MODULE ((uint8_t)255)

#include <MI/ModuleInterface.h>
#include <utils/MITime.h>
#include <utils/MIUtilities.h>

class ModuleInterfaceSet {
protected:
  char moduleset_prefix[MVAR_PREFIX_LENGTH+1]; // A unique lower case prefix, useful if there are multiple masters connected to same db
  bool updated_intermodule_dependencies = false; 
  uint16_t active_contract_count = 0;
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
        mvs_out_of_memory = true;
        #ifdef DEBUG_PRINT
        DPRINTLN(F("MIS::constr OUT OF MEMORY"));
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
	  uint16_t count = count_active_contracts();
	  if (count != active_contract_count) updated_intermodule_dependencies = false;
    if (!updated_intermodule_dependencies && count > 1) {
      for (uint8_t i = 0; i < num_interfaces; i++) {
        interfaces[i]->allocate_source_arrays();
        for (int j=0; j<interfaces[i]->inputs.get_num_variables(); j++) {
          find_output_by_name(interfaces[i]->inputs.get_module_variable(j).name,
                              interfaces[i]->input_source_module_ix[j],
                              interfaces[i]->input_source_output_ix[j]);
        }
      }
      updated_intermodule_dependencies = true;
      active_contract_count = count;
    }
  }
  
  // If there are inter-module dependencies, then transfer values between modules (outputs to inputs)
  void transfer_outputs_to_inputs() {
    if (!got_all_contracts()) {
      #ifdef DEBUG_PRINT
      DPRINTLN(F("** ALL CONTRACTS NOT SET"));
      #endif
      return;
    }
    update_intermodule_dependencies();	
    if (!updated_intermodule_dependencies) return; // Not updated yet, wait for all contracts
    uint8_t buf[4]; // Largest value possibly encountered
    for (int i=0; i<num_interfaces; i++) {
      if (interfaces[i]->input_source_module_ix.length()==0 || interfaces[i]->input_source_output_ix.length() == 0) continue;
      bool all_sources_updated = true, some_set = false;
      for (int j=0; j<interfaces[i]->inputs.get_num_variables(); j++) {
        uint8_t module_ix = interfaces[i]->input_source_module_ix[j], var_ix = interfaces[i]->input_source_output_ix[j];
        if (module_ix != NO_MODULE && var_ix != NO_VARIABLE) {
          if (interfaces[module_ix]->outputs.is_updated()) {     
            uint8_t size = interfaces[i]->inputs.get_module_variable(j).get_size();
            memset(buf, 0, 4);
            interfaces[module_ix]->outputs.get_value(var_ix, buf, size);
            interfaces[i]->inputs.set_value(j, buf, size);
            some_set = true;
          } else all_sources_updated = false; // Source not updated yet
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
            DPRINT(F("Inputs NOT UPDATED for interface ")); DPRINT(i); DPRINTLN(F(" because of old source outputs."));
          }
        }
        #endif
      }
    }
  }
  
  // If any outputs have been flagged as events, also set value and the event flag on inputs that are using them.
  void transfer_events_from_outputs_to_inputs() {
    if (!updated_intermodule_dependencies) return; // Not updated yet, wait for all contracts
    uint8_t buf[4]; // Largest value possibly encountered
    for (uint8_t i = 0; i < num_interfaces; i++) {
      if (interfaces[i]->input_source_module_ix.length()==0 || interfaces[i]->input_source_output_ix.length() == 0) continue;
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

  uint16_t count_active_contracts() {
	uint16_t count = 0;  
    for (uint8_t i = 0; i < num_interfaces; i++) {
      if (interfaces[i]->got_contract() && interfaces[i]->is_active()) count++;
    }
    return count;
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
  
  // Locate the interface that has the specified prefix. Only the start of the given string will be checked,
  // so it can be a full prefixed variable name.
  uint8_t find_interface_by_prefix(const char *prefix) const {
    for (uint8_t i = 0; i < num_interfaces; i++)
      if (strncmp(prefix, interfaces[i]->get_prefix(), MVAR_PREFIX_LENGTH)==0) return i;
    return NO_MODULE;
  }

  // Helper functions for getting a variable set for a specific interface
  ModuleVariableSet *find_settings_by_prefix(const char *prefix) {
    uint8_t i = find_interface_by_prefix(prefix);
    return i == NO_MODULE ? NULL : &(interfaces[i]->settings);
  }
  ModuleVariableSet *find_inputs_by_prefix(const char *prefix) {
    uint8_t i = find_interface_by_prefix(prefix);
    return i == NO_MODULE ? NULL : &(interfaces[i]->inputs);
  }
  ModuleVariableSet *find_outputs_by_prefix(const char *prefix) {
    uint8_t i = find_interface_by_prefix(prefix);
    return i == NO_MODULE ? NULL : &(interfaces[i]->outputs);
  }

  bool find_output_by_name(const char *name, uint8_t &interface_ix, uint8_t &output_ix) const {
    char prefixed_name[MVAR_MAX_NAME_LENGTH + MVAR_PREFIX_LENGTH + 1];
    for (uint8_t i=0; i<num_interfaces; i++) {
      for (uint8_t j=0; j < interfaces[i]->outputs.get_num_variables(); j++) {
        interfaces[i]->outputs.get_module_variable(j).get_prefixed_name(interfaces[i]->get_prefix(), prefixed_name, sizeof prefixed_name);
        if (strcmp(name, prefixed_name) == 0) {
          interface_ix = i;
          output_ix = j;
          return true;
        }
      }
    }
    interface_ix = NO_MODULE;
    output_ix = NO_VARIABLE;
    return false;
  }

  bool find_setting_by_name(const char *name, uint8_t &interface_ix, uint8_t &setting_ix) const {
    interface_ix = find_interface_by_prefix(name);
    if (interface_ix != NO_MODULE && strlen(name) > MVAR_PREFIX_LENGTH) {
      setting_ix = interfaces[interface_ix]->settings.get_variable_ix(&name[MVAR_PREFIX_LENGTH]);
      if (setting_ix != NO_VARIABLE) return true;
    }
    interface_ix = NO_MODULE;
    setting_ix = NO_VARIABLE;
    return false;
  }
};
