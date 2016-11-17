#pragma once

#include <ModuleVariableSet.h>

// Commands for transferring information between modules via some protocol
enum ModuleCommand {
  mcUnknownCommand,
  
  mcSendSettingContract,
  mcSendInputContract,
  mcSendOutputContract,

  mcSetSettingContract,
  mcSetInputContract,
  mcSetOutputContract,

  mcSendSettings,
  mcSendInputs,
  mcSendOutputs,  
  mcSendStatus, // Master is asking regularly if the module has anything to report that is not available in the variable sets
  
  mcSetSettings,
  mcSetInputs,
  mcSetOutputs,
  mcSetStatus,
  
  mcSetTime
};

#define MAX_MODULE_NAME_LENGTH 8

// Status bits
#define CONTRACT_MISMATCH_SETTINGS 1 // We received settings for another version, ask Master to update contract
#define CONTRACT_MISMATCH_INPUTS 2   // We received inputs for another version, ask Master to update contract
#define MISSING_SETTINGS 4           // Tell master that we need settings
#define MISSING_INPUTS 8             // Tell master that we need inputs
#define MODIFIED_SETTINGS 16         // Tell master that the settings have been modified in module, and should be retrieved
#define MISSING_TIME 32              // Tell master that we need a time update (usually only at startup, broadcast should keep it in sync)

// Notification types for the notification callback function
enum NotificationType {
  ntUnknown,
  
  ntNewSettingContract, // Called after new settings have been received (module side and possibly master side)
  ntNewInputContract,   // Called after new inputs have been received (module side)
  ntNewOutputContract,  // Called after new outputs have been received (master side)
  
  ntNewSettings,        // Called after new settings have been received (module side and possibly master side)
  ntNewInputs,          // Called after new inputs have been received (module side)
  ntNewOutputs,         // Called after new outputs have been received (master side)
  ntNewStatus,          // Called after new status has been received (master side)
  ntNewTime,            // Called after new time has been received (module side)
  
  ntSampleSettings,     // Called before inputs are sent to module (master side)
  ntSampleInputs,       // Called before inputs are sent to module (master side)
  ntSampleOutputs,      // Called before outputs are sent to master, allowing for immediate sampling of sensors (module side)
  ntSampleStatus,       // Called before status is sent to master, allowing for immediate sampling of status (module side)
  ntSampleTime          // Called before sending time to module (master side)
};

// Notification callback
class ModuleInterface;
typedef void (*notify_function)(NotificationType notification_type, const ModuleInterface *module_interface);

void dummy_notification_function(NotificationType notification_type, const ModuleInterface *module_interface) {};


class ModuleInterface {
public:
  char module_name[MAX_MODULE_NAME_LENGTH+1];  // A user readable name for the module
  uint8_t status_bits = 0;     // Bits for requesting transfer of contracts or values from master
  ModuleVariableSet settings,  // Configuration parameters needed by the module to operate
                    inputs,    // Measurements / online values needed as input to the module
                    outputs;   // Measurements / online values delivered by the module
  uint32_t last_alive = 0;     // time of last life-sign (reply or ACK) for this module
  uint32_t up_time = 0;        // Uptime in seconds
  uint32_t last_uptime_millis = 0; // Millis when uptime was incremented last
  #ifdef IS_MASTER                    
  char module_prefix[MVAR_PREFIX_LENGTH+1];    // A unique lower case prefix for the module, separating it from other modules
  uint8_t comm_failures = 0;   // If a module is unreachable it can be temporarily deactivated
  bool out_of_memory = false;  // If a module has reached an out-of-memory exception (but still can report back)
  ModuleVariableSet *confirmed_settings = NULL; // Configuration parameters received from the module
  #endif
    
  // Time sync support
  #ifndef NO_TIME_SYNC
  #ifndef IS_MASTER
  // Let time be synchronized from master
  uint32_t time_utc_s = 0,               // Current UTC time (or 0 if it has not been set)
           time_utc_incremented_ms = 0,  // Last millis() when time_utc_s was incremented
           time_utc_received_s = 0,      // UTC time received last time, not incremented.
           time_utc_startup_s = 0;       // Startup time, for calculating uptime
  #endif
  #endif
  
  ModuleInterface() { 
    init(); 
    module_name[0] = 0; 
    #ifdef IS_MASTER
    module_prefix[0] = 0;
    #endif
  }
    
  // Constructors for master side
  #ifdef IS_MASTER  
  ModuleInterface(const char *module_name, const char *prefix) {
    init(); set_name(module_name); set_prefix(prefix);
  }
  #endif
  
  // Constructor for module side
  #ifndef IS_MASTER
  // Specify contracts to constructor...
  ModuleInterface(const char *module_name, const char *settingnames, const char *inputnames, const char *outputnames) { 
   init(); set_contracts(module_name, settingnames, inputnames, outputnames);
  }
  // ... or just preallocate variables, then use set_contracts to configure
  ModuleInterface(const uint8_t num_settings, const uint8_t num_inputs, const uint8_t num_outputs) {
    init(); preallocate_variables(num_settings, num_inputs, num_outputs);
  }
  void set_contracts(const char *module_name, const char *settingnames, const char *inputnames, const char *outputnames) {
    set_name(module_name);
    settings.set_variables(settingnames);
    inputs.set_variables(inputnames);
    outputs.set_variables(outputnames);
  }
  // This function can be called early on to pre-allocate module variables to avoid memory fragmentation.
  // Either specificy variables to constructor in a global declaration, or call this function before allocating strings to send
  // to set_contracts from within a function.
  // If this function returns false, there is a fatal memory problem and the program should be aborted
  bool preallocate_variables(const uint8_t num_settings, const uint8_t num_inputs, const uint8_t num_outputs) {
    return settings.preallocate_variables(num_settings) &&
           inputs.preallocate_variables(num_inputs) &&
           outputs.preallocate_variables(num_outputs);
  }
  #endif
  
  void init() {
    #ifndef IS_MASTER
    status_bits = MISSING_SETTINGS | MISSING_INPUTS; // We want new settings and inputs after a restart
    #ifndef NO_TIME_SYNC
    status_bits |= MISSING_TIME;
    #endif
    #endif
  }
  
  void set_name(const char *name) {
    uint8_t len = min(strlen(name), MAX_MODULE_NAME_LENGTH);
    strncpy(module_name, name, len);
    module_name[len] = 0; // Null-terminate
  }
  
  // Return the time passed since the last life sign was received from module, or -1 if no life sign received after startup
  int32_t get_last_alive_age() { return last_alive ? (uint32_t) ((millis() - last_alive))/1000 : -1; }  
  
  #ifdef IS_MASTER
  void set_prefix(const char *prefix) {
    uint8_t len = prefix ? min(strlen(prefix), MVAR_PREFIX_LENGTH) : 0;
    strncpy(module_prefix, prefix, len);
    module_prefix[len] = 0; // Null-terminate
  }
  const char *get_prefix() const { return module_prefix; }
  bool got_prefix() const { return module_prefix[0] != 0; }
  bool got_contract() const { return settings.got_contract() && inputs.got_contract() && outputs.got_contract(); }
  #endif
  
  // Try to parse and handle an incoming message without response, return true if handled
  bool handle_input_message(const uint8_t *message, const uint8_t length) {  
    if (length < 1) return false;
    #ifdef DEBUG_PRINT
      Serial.print(F("INPUT TYPE ")); Serial.print(message[0]); Serial.print(F(", len ")); Serial.println(length);
    #endif 
    #ifdef IS_MASTER
      comm_failures = 0;
    #endif
    last_alive = millis();
    switch(message[0]) {
      #ifdef IS_MASTER
      case mcSetSettingContract: 
        settings.set_variables(&message[1], length-1); 
        status_bits &= ~CONTRACT_MISMATCH_SETTINGS;
        notify(ntNewSettingContract, this);
        break;
      case mcSetInputContract: 
        inputs.set_variables(&message[1], length-1);
        status_bits &= ~CONTRACT_MISMATCH_INPUTS; 
        notify(ntNewInputContract, this);
        break;
      case mcSetOutputContract: 
        outputs.set_variables(&message[1], length-1);
        notify(ntNewOutputContract, this);
        break;
      case mcSetOutputs: 
        outputs.set_values(&message[1], length-1);
        if (outputs.is_updated()) notify(ntNewOutputs, this);
        break;
      case mcSetStatus: set_status(&message[1], length-1); notify(ntNewStatus, this); break;
      #endif
      case mcSetSettings: 
        if (!settings.set_values(&message[1], length-1)) // Set or clear contract mismatch bit depending on success
           status_bits |= CONTRACT_MISMATCH_SETTINGS; 
         else {
           status_bits &= ~CONTRACT_MISMATCH_SETTINGS; // Clear "missing settings contract" flag
           if (settings.is_updated()) {
             status_bits &= ~MISSING_SETTINGS; // Clear "missing settings" flag
             notify(ntNewSettings, this);
           }
         }
         break;
      #ifndef IS_MASTER
      case mcSetInputs: 
        if (!inputs.set_values(&message[1], length-1)) // Set or clear contract mismatch bit depending on success
           status_bits |= CONTRACT_MISMATCH_INPUTS;
         else {
           status_bits &= ~CONTRACT_MISMATCH_INPUTS; // Clear "missing inputs contract" flag
           if (inputs.is_updated()) {
             status_bits &= ~MISSING_INPUTS; // Clear "missing inputs" flag
             notify(ntNewInputs, this);
           }
         }
         break;
      case mcSetTime: set_time(&message[1], length-1); notify(ntNewTime, this); break;
      #endif
      default: return false; // Unrecognized message
    }
    #ifdef DEBUG_PRINT
      if (message[0] == mcSetSettingContract) {
        Serial.print(F("Settings contract: "));
        settings.debug_print_contract();
      }
      else if (message[0] == mcSetInputContract) {
        Serial.print(F("Inputs contract: "));
        inputs.debug_print_contract();
      }
      else if (message[0] == mcSetOutputContract) {
        Serial.print(F("Outputs contract: "));
        outputs.debug_print_contract();
      }
      else if (message[0] == mcSetSettings) {
        Serial.print(F("Settings: "));
        settings.debug_print_values();
      }
      else if (message[0] == mcSetInputs) {
        Serial.print(F("Inputs: "));
        inputs.debug_print_values();
      }
      else if (message[0] == mcSetOutputs) {
        Serial.print(F("Outputs: "));
        outputs.debug_print_values();
      }
      else if (message[0] == mcSetStatus) {
        Serial.print(F("Status: ")); Serial.println(message[1]);
      }
    #endif      
    return true;
  }
  
  // Try to parse it as a request for data, returning the data in response if returning true
  bool handle_request_message(const uint8_t *message, const uint8_t length, BinaryBuffer &response, uint8_t &response_length) {
    response_length = 0;
    if (length < 1) return false;
    #ifdef DEBUG_PRINT
      Serial.print("REQUEST TYPE "); Serial.print(message[0]); Serial.print(", len "); Serial.println(length);
    #endif
    #ifdef IS_MASTER
      comm_failures = 0;
    #endif
    last_alive = millis();
    switch(message[0]) {
      #ifndef IS_MASTER
      case mcSendSettingContract: 
        settings.get_variables(response, response_length, mcSetSettingContract); 
        status_bits &= ~CONTRACT_MISMATCH_SETTINGS; 
        break; // Requested by master, so clear the bit
      case mcSendInputContract:   
        inputs.get_variables(response, response_length, mcSetInputContract);
        status_bits &= ~CONTRACT_MISMATCH_INPUTS;
        break; // Requested by master, so clear the bit
      case mcSendOutputContract:    
        outputs.get_variables(response, response_length, mcSetOutputContract);
        break;
      case mcSendOutputs:
        notify(ntSampleOutputs, this); 
        outputs.get_values(response, response_length, mcSetOutputs); 
        break;
      case mcSendStatus:
        notify(ntSampleStatus, this);      
        get_status(response, response_length); 
        break;
      #endif  
      case mcSendSettings:
        notify(ntSampleSettings, this);      
        settings.get_values(response, response_length, mcSetSettings);  // TODO: use MODIFIED_SETTINGS bit for modules with a GUI
        break;
      #ifdef IS_MASTER          
      case mcSendInputs:          
        notify(ntSampleInputs, this);
        inputs.get_values(response, response_length, mcSetInputs); 
        break;
      #endif  
      default: return false; // Unrecognized message
    }
    return true;
  }
  
  // Get the status for a module
  uint8_t get_status_bits() const { return status_bits; }

  // Register notification callback function
  void set_notification_callback(notify_function n) { notify = n; }

  #ifndef IS_MASTER
  #ifndef NO_TIME_SYNC
  // Has time been set in the not too far past?
  bool is_time_set() {
    update_time();
    return (time_utc_received_s != 0 && time_utc_s != 0 && ((uint32_t)(time_utc_s - time_utc_received_s) < 48ul*3600ul));
  }

  // Get UTC time
  uint32_t get_time_utc_s() {
    update_time();
    return time_utc_received_s ? time_utc_s : 0;
  }
  #endif !NO_TIME_SYNC
  #endif !IS_MASTER  

  // Return the uptime in seconds of this module
  uint32_t get_uptime_s() {
    #ifdef IS_MASTER
    return up_time;
    #else // IS_MASTER
    // Calculate an accumulated uptime in seconds from system clock in milliseconds,
    // taking rollover into account.
    uint32_t t = millis(), diff = (uint32_t)(t - last_uptime_millis);
    if (diff > 1000) {
      diff /= 1000;
      up_time += diff;
      last_uptime_millis += diff*1000;
    }
    #ifndef NO_TIME_SYNC // Expecting time sync, should use UTC time minus startup UTC
    update_time();
    if (time_utc_received_s) {
      if (!time_utc_startup_s) time_utc_startup_s = time_utc_s - millis()/1000ul;
      return time_utc_s - time_utc_startup_s;
    }
    #endif // !NO_TIME_SYNC
    return up_time; // If time sync disabled or not received, return accumulated calculated uptime
    #endif // IS_MASTER
  }  
  
protected:  

friend class ModuleInterfaceSet; 

  notify_function notify = dummy_notification_function;
    
  #ifndef IS_MASTER
  void get_status(BinaryBuffer &message, uint8_t &length) {
    if (message.allocate(7)) {
      message.get()[0] = mcSetStatus;
      message.get()[1] = (uint8_t) status_bits; 
      message.get()[2] = (uint8_t) ModuleVariableSet::out_of_memory; 
      uint32_t uptime_s = get_uptime_s();   
      memcpy(&(message.get()[3]), &uptime_s, sizeof uptime_s);
      length = 7;
    } else { length = 0; ModuleVariableSet::out_of_memory = true; }
  }
  
  // This must be called regularly to keep the time updated (at least once for each each millis() rollover)
  #ifndef NO_TIME_SYNC
  void update_time() {
    if (time_utc_received_s) {
      uint32_t elapsed_s = ((uint32_t)(millis() - time_utc_incremented_ms)) / 1000ul;
      if (elapsed_s > 0) {
        if (elapsed_s > 12ul*3600ul) status_bits |= MISSING_TIME; // Explicitly request new time sync if too long since the last sync
        time_utc_s += elapsed_s;
        time_utc_incremented_ms += elapsed_s * 1000ul;
      }
    } else status_bits |= MISSING_TIME;
  }
  #endif
  
  // Support setting time automatically if the Time.h header file is included before this
  void set_time(const uint8_t *message, const uint8_t length) {
    #ifndef NO_TIME_SYNC
    if (length == 4) {
      #ifdef DEBUG_PRINT
        uint32_t initial_time = get_time_utc_s();
      #endif
      time_utc_incremented_ms = millis(); // Remember when it was received so it can be auto-incremented
      time_utc_s = *(uint32_t*)message; 
      time_utc_received_s = time_utc_s; // Remember what time was received last
      status_bits &= ~MISSING_TIME; // Clear the missing-time bit
      #ifdef _Time_h
        setTime(time_utc_s);  // Adjust "system clock" in UTC as well if the Time library is used
      #endif
      #ifdef DEBUG_PRINT
        Serial.print(F("Time adjusted by ")); Serial.print((uint32_t) (time_utc_s - initial_time));
        Serial.print(F("s to UTC ")); Serial.println(time_utc_s);
      #endif
    }
    #endif
  }  
  #endif // !IS_MASTER
  
  #ifdef IS_MASTER  
  void set_status(const uint8_t *message, const uint8_t length) {
    if (length == 6) {
      last_alive = millis();
      comm_failures = 0;
      status_bits = message[0];
      out_of_memory = message[1];
      memcpy(&up_time, &message[2], sizeof up_time);
      if (got_contract() && settings.is_master() && (status_bits & (CONTRACT_MISMATCH_SETTINGS | CONTRACT_MISMATCH_INPUTS))) {
        // Module flags that it does not have the same contract, usually for settings or inputs. Invalidate relevant part.
        #ifdef DEBUG_PRINT
          Serial.print(F("Module changed contract. Invalidating ")); Serial.println(status_bits);           
        #endif  
        if (status_bits & CONTRACT_MISMATCH_SETTINGS) settings.invalidate_contract();
        if (status_bits & CONTRACT_MISMATCH_INPUTS) inputs.invalidate_contract();
      }
    }
  }  
  
  // These are used by ModuleInterfaceSet to manage inter-module value transfer
 
  BinaryBuffer input_source_module_ix,
               input_source_output_ix;
  void allocate_source_arrays() {
    if (input_source_module_ix.allocate(inputs.get_num_variables()) &&
        input_source_output_ix.allocate(inputs.get_num_variables())) {
      input_source_module_ix.set_all(NO_VARIABLE); // no source
      input_source_output_ix.set_all(NO_VARIABLE); // no source
    } else ModuleVariableSet::out_of_memory = true;
  }
  #endif  
};


