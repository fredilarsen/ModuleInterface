#pragma once

#include <MI/ModuleVariableSet.h>

// Commands for transferring information between modules via some protocol
enum ModuleCommand {
  mcUnknownCommand,

  mcSendSettingContract,  // 1
  mcSendInputContract,
  mcSendOutputContract,

  mcSetSettingContract,   // 4
  mcSetInputContract,
  mcSetOutputContract,

  mcSendSettings,         // 7
  mcSendInputs,
  mcSendOutputs,
  mcSendStatus, // Master is asking regularly if the module has anything to report that is not available in the variable sets

  mcSetSettings,          // 11
  mcSetInputs,
  mcSetOutputs,
  mcSetStatus,

  mcSetTime               // 15
};

#define MAX_MODULE_NAME_LENGTH 8
#define MI_INACTIVE_THRESHOLD  5       // The number of consecutive failed transfers before module is deactivated
#define MI_INACTIVE_TIME_THRESHOLD 120 // The number of seconds without life sign before module is deactivated

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
extern void dummy_notification_function(NotificationType notification_type, const ModuleInterface *module_interface);

#define TIME_UTC_2017 1483228800ul

class ModuleInterface {
  #ifndef IS_MASTER
  static const char *settings_contract, // Pointer to ordinary or PROGMEM string constant
                    *inputs_contract,
                    *outputs_contract;
  #endif
public:
  char module_name[MAX_MODULE_NAME_LENGTH+1];  // A user readable name for the module
  uint8_t status_bits = 0;     // Bits for requesting transfer of contracts or values from master
  ModuleVariableSet settings,  // Configuration parameters needed by the module to operate
                    inputs,    // Measurements / online values needed as input to the module
                    outputs;   // Measurements / online values delivered by the module
  uint32_t last_alive = 0;     // time of last life-sign (reply but not ACK) for this module
  uint32_t up_time = 0;        // Uptime in seconds
  uint32_t last_uptime_millis = 0; // Millis when uptime was incremented last
  static ModuleInterface *singleton;
  #ifdef IS_MASTER
  char module_prefix[MVAR_PREFIX_LENGTH+1];    // A unique lower case prefix for the module, separating it from other modules
  uint8_t comm_failures = 0;   // If a module is unreachable it can be temporarily deactivated
  bool out_of_memory = false;  // If a module has reached an out-of-memory exception (but still can report back)
  ModuleVariableSet *confirmed_settings = NULL; // Configuration parameters received from the module
  ModuleCommand last_incoming_cmd = mcUnknownCommand;  // Cmd in last received packet
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
  #ifdef MI_NO_DYNAMIC_MEM
  void set_variables(uint8_t num_settings, ModuleVariable *setting_variables,
                     uint8_t num_inputs,   ModuleVariable *input_variables,
                     uint8_t num_outputs,  ModuleVariable *output_variables) {
    settings.set_variables(num_settings, setting_variables);
    inputs.set_variables(num_inputs, input_variables);
    outputs.set_variables(num_outputs, output_variables);
  }
  #else
  // Specify contracts to constructor by providing string constants (must remain accessible, they will not be copied)...
  ModuleInterface(const char *module_name,
                  const char *settingnames,
                  const char *inputnames,
                  const char *outputnames) {
    init();
    set_contracts(module_name, settingnames, inputnames, outputnames);
  }
  // Specify contracts to constructor by providing PROGMEM constants...
  ModuleInterface(const char *module_name,
                  const bool use_progmem, // Must be true, this is a requirement
                  const char * PROGMEM settingnames,
                  const char * PROGMEM inputnames,
                  const char * PROGMEM outputnames) {
    init();
    if (use_progmem) set_contracts_P(module_name, settingnames, inputnames, outputnames);
  }
  // Specify contracts to constructor by providing callback functions...
  ModuleInterface(const char                *module_name,
                        MVS_getContractChar settingnames,
                        MVS_getContractChar inputnames,
                        MVS_getContractChar outputnames) {
    init();
    set_contracts(module_name, settingnames, inputnames, outputnames);
  }
  // ... or just preallocate variables, then use set_contracts to configure
  ModuleInterface(const uint8_t num_settings, const uint8_t num_inputs, const uint8_t num_outputs) {
    init(); preallocate_variables(num_settings, num_inputs, num_outputs);
  }
  #endif

  void set_contracts(const char *module_name,
                     const char *settingnames,
                     const char *inputnames,
                     const char *outputnames) {
    // Remember pointers to the strings
    settings_contract = settingnames;
    inputs_contract   = inputnames;
    outputs_contract  = outputnames;

    // Register the callbacks that relate to an ordinary string
    set_contracts(module_name, settings_callback, inputs_callback, outputs_callback);
  }
  void set_contracts_P(const char *module_name,
                       const char * PROGMEM settingnames,
                       const char * PROGMEM inputnames,
                       const char * PROGMEM outputnames) {
    // Remember pointers to the strings
    settings_contract = settingnames;
    inputs_contract   = inputnames;
    outputs_contract  = outputnames;

    // Register the callbacks that relate to a PROGMEM string
    set_contracts(module_name, settings_callback_P, inputs_callback_P, outputs_callback_P);
  }
  void set_contracts(const char *module_name,
                           MVS_getContractChar settingnames_callback,
                           MVS_getContractChar inputnames_callback,
                           MVS_getContractChar outputnames_callback) {
    set_name(module_name);

    // Register the user-specified callbacks
    settings.set_variables_by_callback(settingnames_callback);
    inputs.set_variables_by_callback(inputnames_callback);
    outputs.set_variables_by_callback(outputnames_callback);

    if (settings.get_num_variables() == 0) status_bits &= !MISSING_SETTINGS; // No settings, so do not mark them as missing
    if (inputs.get_num_variables() == 0) status_bits &= !MISSING_INPUTS;     // No inputs, so do not mark them as missing
  }
  #ifndef MI_NO_DYNAMIC_MEM
  // This function can be called early on to pre-allocate module variables to avoid memory fragmentation.
  // Either specificy variables to constructor in a global declaration, or call this function before allocating strings to send
  // to set_contracts from within a function.
  // If this function returns false, there is a fatal memory problem and the program should be aborted
  bool preallocate_variables(const uint8_t num_settings, const uint8_t num_inputs, const uint8_t num_outputs) {
    if (num_settings == 0) status_bits &= !MISSING_SETTINGS; // No settings, so do not mark them as missing
    if (num_inputs == 0) status_bits &= !MISSING_INPUTS;     // No inputs, so do not mark them as missing
    return settings.preallocate_variables(num_settings) &&
           inputs.preallocate_variables(num_inputs) &&
           outputs.preallocate_variables(num_outputs);
  }
  #endif
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
    strncpy(module_name, name, MAX_MODULE_NAME_LENGTH);
    module_name[MAX_MODULE_NAME_LENGTH] = 0; // Null-terminate
  }

  #ifdef DEBUG_PRINT
  // To ease prefixing all debug messages with module name
  void dname() {
    #ifdef IS_MASTER
    Serial.print(module_name); Serial.print(": ");
    #endif
  }
  #endif

  // Return the time passed since the last life sign was received from module, or -1 if no life sign received after startup
  int32_t get_last_alive_age() const { return last_alive ? (uint32_t) ((millis() - last_alive))/1000ul : -1; }

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
      dname(); Serial.print(F("INPUT TYPE ")); Serial.print(message[0]); Serial.print(F(", len ")); Serial.println(length);
    #endif
    #ifdef IS_MASTER
      comm_failures = 0;
    #endif
    last_alive = millis(); if (last_alive == 0) last_alive = 1;
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
        dname(); Serial.print(F("Settings contract: "));
        settings.debug_print_contract();
      }
      else if (message[0] == mcSetInputContract) {
        dname(); Serial.print(F("Inputs contract: "));
        inputs.debug_print_contract();
      }
      else if (message[0] == mcSetOutputContract) {
        dname(); Serial.print(F("Outputs contract: "));
        outputs.debug_print_contract();
      }
      else if (message[0] == mcSetSettings) {
        dname(); Serial.print(F("Settings: "));
        settings.debug_print_values();
      }
      else if (message[0] == mcSetInputs) {
        dname(); Serial.print(F("Inputs: "));
        inputs.debug_print_values();
      }
      else if (message[0] == mcSetOutputs) {
        dname(); Serial.print(F("Outputs: "));
        outputs.debug_print_values();
      }
      else if (message[0] == mcSetStatus) {
        dname(); Serial.print(F("Status: ")); Serial.println(message[1]);
      }
    #endif
    return true;
  }

  // Try to parse it as a request for data, returning the data in response if returning true
  bool handle_request_message(const uint8_t *message, const uint8_t length, BinaryBuffer &response, uint8_t &response_length) {
    response_length = 0;
    if (length < 1) return false;
    #ifdef DEBUG_PRINT
      dname(); Serial.print("REQUEST "); Serial.print(message[0]); Serial.print(", len "); Serial.println(length);
    #endif
    #ifdef IS_MASTER
      comm_failures = 0;
    #endif
    last_alive = millis(); if (last_alive == 0) last_alive = 1;
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
        outputs.clear_events();  // Will be transferred normally, so clear event flag
        outputs.clear_changed(); // Changes are detected between each transfer
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

    #ifdef DEBUG_PRINT
    if (message[0] == mcSendSettings) { // Can be called for master, and for module if it has own GUI
      dname(); Serial.print(F("Send Settings: "));
      settings.debug_print_values();
    } else
    #ifdef IS_MASTER
    if (message[0] == mcSendInputs) {
      dname(); Serial.print(F("Send Inputs: "));
      inputs.debug_print_values();
    }
    #else
    if (message[0] == mcSendSettingContract) {
      dname(); Serial.print(F("Send Settings contract: "));
      settings.debug_print_contract();
    } else
    if (message[0] == mcSendInputContract) {
      dname(); Serial.print(F("Send Inputs contract: "));
      inputs.debug_print_contract();
    } else
    if (message[0] == mcSendOutputContract) {
      dname(); Serial.print(F("Send Outputs contract: "));
      outputs.debug_print_contract();
    } else
    if (message[0] == mcSendOutputs) {
      dname(); Serial.print(F("Send Outputs: "));
      outputs.debug_print_values();
    } else
    if (message[0] == mcSendStatus) {
      dname(); Serial.print(F("Send Status: ")); Serial.println(status_bits);
    }
    #endif
    #endif
    return true;
  }

  // Get the status for a module
  uint8_t get_status_bits() const { return status_bits; }

  // Register notification callback function
  void set_notification_callback(notify_function n) { notify = n; }

  // Whether this module is active or has not been reachable for a while
  bool is_active() const {
    #ifdef IS_MASTER
    return comm_failures < (uint8_t)MI_INACTIVE_THRESHOLD && get_last_alive_age() != -1 && get_last_alive_age() < 1000L*MI_INACTIVE_TIME_THRESHOLD;
    #else
    return true;
    #endif
  }

  #ifndef IS_MASTER
  #ifndef NO_TIME_SYNC
  // Has time been set in the not too far past?
  bool is_time_set() {
    update_time();
    return (time_utc_received_s != 0 && time_utc_s != 0 && time_utc_s > TIME_UTC_2017
           && ((uint32_t)(time_utc_s - time_utc_received_s) < 48ul*3600ul)); // Synced not more than 2 days ago
  }

  // Get UTC time
  uint32_t get_time_utc_s() {
    update_time();
    return time_utc_received_s ? time_utc_s : 0;
  }
  #endif // !NO_TIME_SYNC
  #endif // !IS_MASTER

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
    } else {
      length = 0; ModuleVariableSet::out_of_memory = true;
      #ifdef DEBUG_PRINT
      Serial.println(F("MI::get_status OUT OF MEMORY"));
      #endif
    }
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

  // Receiving time sync from master
  void set_time(const uint8_t *message, const uint8_t length) {
    #ifndef NO_TIME_SYNC
    if (length == 4 && *(uint32_t*)message > TIME_UTC_2017) {
      #ifdef DEBUG_PRINT
        uint32_t initial_time = get_time_utc_s();
      #endif
      time_utc_incremented_ms = millis(); // Remember when it was received so it can be auto-incremented
      time_utc_s = *(uint32_t*)message;
      time_utc_received_s = time_utc_s; // Remember what time was received last
      status_bits &= ~MISSING_TIME; // Clear the missing-time bit
      #ifdef DEBUG_PRINT
        dname(); Serial.print(F("Time adjusted by ")); Serial.print((uint32_t) (time_utc_s - initial_time));
        Serial.print(F("s to UTC ")); Serial.println(time_utc_s);
      #endif
    }
    #endif
  }
  #endif // !IS_MASTER

  #ifdef IS_MASTER
  void set_status(const uint8_t *message, const uint8_t length) {
    if (length == 6) {
      last_alive = millis(); if (last_alive == 0) last_alive = 1;
      comm_failures = 0;
      status_bits = message[0];
      out_of_memory = message[1];
      memcpy(&up_time, &message[2], sizeof up_time);
      if (got_contract() && (status_bits & (CONTRACT_MISMATCH_SETTINGS | CONTRACT_MISMATCH_INPUTS))) {
        // Module flags that it does not have the same contract, usually for settings or inputs. Invalidate relevant part.
        #ifdef DEBUG_PRINT
          dname(); Serial.print(F("Module changed contract. Invalidating ")); Serial.println(status_bits);
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
    } else {
      ModuleVariableSet::out_of_memory = true;
      #ifdef DEBUG_PRINT
      Serial.println(F("MI::allocate_source_arrays OUT OF MEMORY"));
      #endif
    }
  }
  #endif

  // Callbacks for reading contracts from ordinary string constants
  static char settings_callback(uint16_t pos);
  static char inputs_callback(uint16_t pos);
  static char outputs_callback(uint16_t pos);

  // Callbacks for reading contracts from PROGMEM string constants
  static char settings_callback_P(uint16_t pos);
  static char inputs_callback_P(uint16_t pos);
  static char outputs_callback_P(uint16_t pos);
};
