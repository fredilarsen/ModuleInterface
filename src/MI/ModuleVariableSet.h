#pragma once

#include <MI/ModuleVariable.h>

// Value returned by get_variable_ix when variable name not found. Means that max 255 variables may be used.
#define NO_VARIABLE 0xFF

// A callback for getting the contract string when needed without keeping the names in RAM.
// It must return the character at the given position in the string, and return char(0) after the last character.
typedef char (* MVS_getContractChar)(uint16_t position);

// This variable object can be used on the master side to handle changing contracts,
// where a variable index may become invalid if the number of parameters or parameter order changes.
// It can be used on the module side as well if the extra bytes of storage/RAM usage is acceptable.
#ifdef USE_MIVARIABLE
class MIVariable {
friend class ModuleVariableSet;
protected:
  char name[MVAR_MAX_NAME_LENGTH + 1];
  uint32_t contract_id = 0;
  uint8_t ix = NO_VARIABLE;
public:
  MIVariable() { name[0] = 0; }
  MIVariable(const char *variable_name) { set_name(variable_name); }

  void set_name(const char *variable_name) {
    contract_id = 0;
    ix = NO_VARIABLE;
    uint8_t len = (uint8_t) MI_min(strlen(variable_name), MVAR_MAX_NAME_LENGTH);
    strncpy(name, variable_name, len);
    name[len] = 0;
  }
  bool is_found() const { return ix != NO_VARIABLE; }
};
#endif

struct ModuleVariableSet {
private:
  uint8_t num_variables = 0;
  ModuleVariable *variables = NULL;
  uint8_t total_value_length = 0;    // Length of all values serialized after another
  uint32_t contract_id = 0;          // Number used for detecting changes in contract
  uint32_t values_received_time = 0; // Set by set_values when setting values
  #ifndef IS_MASTER
  MVS_getContractChar get_contract_callback = NULL;
  #endif

  void deallocate() {
    #ifndef MI_NO_DYNAMIC_MEM
    if (variables) { delete[] variables; variables = NULL; num_variables = 0; }
    #endif
  }

  void calculate_total_value_length() {
    total_value_length = 0;
    for (uint8_t i = 0; i < num_variables; i++) total_value_length += variables[i].get_size();
  }

  uint32_t calculate_contract_id() const { // A contract id that can be used to detect a changed contract
    uint32_t id = 0x33333333; // Non-zero to be able to accept a contract with no variables
    char name_buf[MVAR_COMPOSITE_NAME_LENGTH + 1];
    uint8_t len;
    for (uint8_t i = 0; i < num_variables; i++) {
      #ifdef IS_MASTER
      strncpy(name_buf, variables[i].name, MVAR_MAX_NAME_LENGTH);
      len = (uint8_t) strlen(name_buf);
      #else
      uint16_t source_pos = 0;
      if (!get_next_word_from_contract(source_pos, len, name_buf, sizeof name_buf)) return 0;
      remove_type(name_buf, len);
      #endif
      ((uint8_t*) &id)[0] += variables[i].get_size();
      ((uint8_t*) &id)[1] += len;
      ((uint8_t*) &id)[2] += (uint8_t) variables[i].get_type();
      for (uint8_t j=0; j<len; j++) ((uint8_t*) &id)[3] ^= name_buf[j]; // XOR of chars in names
    }
    // TODO: Use 32 bit CRC of all above instead of sums and XOR?
    return id;
  }

public:
  ModuleVariableSet() { }
  ~ModuleVariableSet() { deallocate(); }

  #ifdef IS_MASTER
  bool get_variable_name(uint8_t ix, char *name_buf) const {
    strncpy(name_buf, variables[ix].name, MVAR_MAX_NAME_LENGTH);
    name_buf[MVAR_MAX_NAME_LENGTH] = 0;
    return true;
  }
  #endif

  #ifndef IS_MASTER
  bool get_next_word_from_contract(uint16_t &source_pos, uint8_t  &word_len, char *name_buf, uint8_t buf_size) const {
    word_len = 0;
    if (get_contract_callback == NULL) return false;
    uint8_t cnt = 0;
    char c;
    do {
      c = get_contract_callback(source_pos);
      if (c != 0) source_pos++;
      if (c != 0 && c != ' ' && cnt < buf_size-1) name_buf[cnt++] = c;
      else { // A complete word is read
        name_buf[cnt] = 0;
        word_len = cnt;
        return word_len > 0;
      }
    } while (c != 0 && c != ' ' && cnt < buf_size-1);
    return false;
  }

  static void remove_type(char *name_buf, uint8_t &name_len) {
    char *colon_pos = strchr(name_buf, ':');
    if (colon_pos != NULL) { *colon_pos = 0; name_len = colon_pos - name_buf; }
  }
  #endif

  // A flag that should be set if memory allocation fails (can be set and read from all places)
  static bool out_of_memory;

  #ifndef IS_MASTER
  #ifdef MI_NO_DYNAMIC_MEM
  // Support for having variables declared in user sketch to avoid dynamic allocation
  void set_variables(uint8_t variable_count, ModuleVariable *variable_array) {
    num_variables = variable_count;
    variables = variable_array;
  }
  #else
  bool preallocate_variables(const uint8_t variable_count) {
    num_variables = variable_count;
    if (variables) { delete[] variables; variables = NULL; }
    if (num_variables == 0) { contract_id = calculate_contract_id(); return true; }
    variables = new ModuleVariable[num_variables];
    if (!variables) {
      out_of_memory = true;
      #ifdef DEBUG_PRINT
      DPRINTLN(F("MVS::preallocate_variables OUT OF MEMORY"));
      #endif
      return false;
    }
    return true;
  }
  #endif

  void set_variables_by_callback(MVS_getContractChar contract_callback) { // Textual format like "var1:u1 var2:f" specified in worker
    // Remember how to get the contract when needed
    get_contract_callback = contract_callback;

    // Parse string, count number of variables
    char name_buf[MVAR_COMPOSITE_NAME_LENGTH + 1];
    uint16_t source_pos = 0;
    uint8_t len, nvar = 0;
    while (get_next_word_from_contract(source_pos, len, name_buf, sizeof name_buf)) nvar++;
    if (nvar > 0) {
      #ifndef MI_NO_DYNAMIC_MEM
      if (num_variables && nvar != num_variables) deallocate(); // Deallocate if num_variables changed
      // Init variables
      if (nvar && !num_variables) variables = new ModuleVariable[nvar];
      num_variables = nvar;
      #endif
      source_pos = 0;
      for (uint8_t i = 0; i < num_variables; i++) {
        if (!get_next_word_from_contract(source_pos, len, name_buf, sizeof name_buf)) {
          deallocate();
          break;
        }
        variables[i].set_variable(name_buf);
      }
    } else deallocate();
    calculate_total_value_length();
    contract_id = calculate_contract_id();
  }
  #endif

  #ifdef IS_MASTER
  void set_variables(const uint8_t *names_and_types, const uint8_t /*length*/) { // Serialized data coming from worker to master
    const uint8_t *p = names_and_types;
    uint32_t new_contract_id = 0;
    memcpy(&new_contract_id, p, 4); p += 4; // Remember incoming contract id
    if (new_contract_id != contract_id) { // If the same contract comes in multiple times, ignore it
      deallocate();
      contract_id = new_contract_id;
      num_variables = *p; p++; // First byte is number of variables
      if (num_variables > 0) {
        variables = new ModuleVariable[num_variables];
        if (variables == NULL) {
          out_of_memory = true;
          #ifdef DEBUG_PRINT
          DPRINT(F("MVS::set_variables OUT OF MEMORY. #var=")); DPRINTLN(num_variables);
          #endif
          return;
        }
      }
      for (uint8_t i = 0; i < num_variables; i++) {
        variables[i].set_variable(p);
        p += (2 + p[1]); // type byte + length byte + name length
      }
      calculate_total_value_length();
    }
    #ifdef DEBUG_PRINT
    else { DPRINT(F("IGNORED DUPLICATE CONTRACT ")); DPRINTLN(contract_id); }
    #endif
  }
  #endif

  #ifndef IS_MASTER
  void get_variables(BinaryBuffer &names_and_types, uint8_t &length, uint8_t header_byte) const {
    // Calculate total buffer size
    length = 6; // Header byte plus Contract id plus number of variables byte
    char name_buf[MVAR_COMPOSITE_NAME_LENGTH + 1];
    uint16_t source_pos = 0;
    uint8_t len;
    for (uint8_t i = 0; i < num_variables; i++) {
      if (!get_next_word_from_contract(source_pos, len, name_buf, sizeof name_buf)) { length = 0; break; }
      remove_type(name_buf, len);
      length += 2 + len; // 2 bytes for type, length
    }
    // Allocate buffer, fill in
    if (names_and_types.allocate(length)) {
      uint8_t *p = names_and_types.get();
      *p = header_byte; p++;
      memcpy(p, &contract_id, 4); p += 4; // Add contract id
      *p = num_variables; p++; // Add number of variables
      source_pos = 0;
      for (uint8_t i = 0; i < num_variables; i++) { // Add each variable type and name
        *p = (uint8_t) variables[i].get_type(); p++;
        if (!get_next_word_from_contract(source_pos, len, name_buf, sizeof name_buf)) { length = 0; break; }
        remove_type(name_buf, len);
        *p = len; p++;
        memcpy(p, name_buf, len);
        p += len;
      }
    } else {
      out_of_memory = true;
      #ifdef DEBUG_PRINT
      DPRINTLN(F("MVS::get_variables OUT OF MEMORY"));
      #endif
    }
  }
  #endif

  void invalidate_contract() {
    #ifdef DEBUG_PRINT
      DPRINTLN(F("***** INVALIDATING CONTRACT"));
    #endif
    #ifdef IS_MASTER
      contract_id = 0;
      values_received_time = 0;
    #endif
  }
  bool got_contract() const { return (contract_id != 0); }
  uint32_t get_contract_id() const { return contract_id; }

  // Returns true if values registered, false if the values do not conform to the current contract id
  bool set_values(const uint8_t *values, const uint8_t length) { // contract_id(4), num_variables(1), <variables>
    // Get number of variables and contract id
    if (length < 5) return false; // Invalid message
    const uint8_t *p = values;
    uint8_t numvar = *(p+4);
    bool event = (numvar & 0b10000000) != 0;
    if (event) numvar = (uint8_t) (numvar & 0b01111111); // Remove event bit
    uint32_t c_id;
    memcpy(&c_id, p, sizeof c_id);    
    if (c_id != contract_id || (numvar > num_variables)) {
      invalidate_contract();
      #ifdef DEBUG_PRINT
        DPRINT(F("Values received with mismatched contract id ")); DPRINTLN(c_id);
      #endif
      return false;
    }
    if (numvar == 0) {
      #ifdef DEBUG_PRINT
	  if (num_variables > 0) {
        DPRINT(F("--> set_values got no values, not updated on sender side yet. Length = ")); DPRINTLN(length);
	  }
      #endif
      return true; // Conforms to contract, but values not available yet
    }
    #ifdef DEBUG_PRINT
    if (numvar < num_variables) {
      DPRINT(F("--> set_values got ")); DPRINT(numvar); DPRINT(F(" of ")); DPRINT(num_variables); DPRINTLN(F(" values."));
    }
    #endif

    // Data corresponds to current contract, so parse values
    p += 5; // Skip over contract id and variable count
    for (uint8_t i = 0; i < numvar && p - values < length; i++) {
      uint8_t varpos = i;
      if (numvar != num_variables) { varpos = *p; p++; } // Variable number included
      if (varpos >= num_variables) {
        #ifdef DEBUG_PRINT
        DPRINTLN(F("--> set_values got corrupted packet"));
        #endif
        return false; // Corrupted message
      }
      uint8_t len = variables[varpos].get_size();
      bool set_value = true;
      #ifdef IS_MASTER
      // In the master, this function is only called when receiving values from a module,
      // so always set the value.
      #else
      // In a module, this function only is called when receiving values from a master or reading from EEPROM.
      // Disallow different values from master as long as the changed-flag is set, and clear the changed-flag
      // when the same value is received from master, indicating that the master has received it.
      if (variables[varpos].is_changed()) {
        if (variables[varpos].is_equal(p, len)) variables[varpos].set_changed(false);
        else set_value = false; // Ignore new values from master as long as changed-flag on module side is set
      } // else // Not changed on module side, so update with new value from master
      #endif
      if (set_value) {
        variables[varpos].set_value(p, len);
        #ifndef IS_MASTER
        variables[varpos].set_changed(false); // Normal flow of values shall not set changed-flag
        #endif
        p += len;
        if (event) variables[varpos].set_event(); // Set event flag on receiving side
      }
    }
    // Flag as updated / ready for use only after we have got a full set
    if (numvar == num_variables) set_updated();
    return true;
  }

  // Get serialized values, usually all, but can be limited to values marked as events
  // and/or values marked as changed. If setting both events_only and changes_only,
  // both will be included.
  void get_values(BinaryBuffer &values, uint8_t &length, uint8_t header_byte,
                  bool events_only = false, bool changes_only = false) const {
    length = 0;

    // Determine the number of variables to be serialized, all or a subset
    uint8_t numvar = num_variables, total_len = total_value_length;
    if (events_only || changes_only) {
      numvar = 0; total_len = 0;
      for (uint8_t i = 0; i < num_variables; i++) {
        if ((events_only && variables[i].is_event()) || (changes_only && variables[i].is_changed())) {
          numvar++;
          total_len +=  variables[i].get_size();
        }
      }
      if (numvar != num_variables) total_len += numvar; // One byte with variable number before each value
      if (numvar == 0) return; // No events or changes to send
    }

    // Serialize
    if (values.allocate(6 + total_len)) {
      // Header
      length = 6 + total_len;
      uint8_t *p = values.get();
      *p = header_byte; p++;
      memcpy(p, &contract_id, 4); p += 4;
      *p = is_updated() || events_only ? numvar : 0; // If values not set yet, then report "no values"
      if (events_only) *p = (uint8_t) (*p | 0b10000000); // Using upper bit for event flag, limiting number of vars to 127
      #ifdef DEBUG_PRINT
        if (!(is_updated() || events_only)) {
          DPRINT(F("Values not updated or event. Sending empty output values. Value length = "));
          DPRINTLN(total_len);
        }
      #endif
      p++;
      // Values
      if (numvar != 0) {
        for (uint8_t i = 0; i < num_variables; i++) {
          if (numvar == num_variables || // include all
            (events_only && variables[i].is_event()) || // event
            (is_updated() && changes_only && variables[i].is_changed())) // changed
          {
            if (numvar != num_variables) { *p = i; p++; } // Variable number if not serializing all
            uint8_t len = variables[i].get_size();
            variables[i].get_value(p, len);
            p += len;
          }
        }
      }
    } else {
      out_of_memory = true;
      #ifdef DEBUG_PRINT
      DPRINTLN(F("MVS::get_values OUT OF MEMORY"));
      #endif
    }
  }

  uint8_t get_num_variables() const { return num_variables; }

  uint8_t get_variable_ix(const char *variable_name) const {
    #ifdef IS_MASTER
    for (uint8_t i = 0; i < num_variables; i++)
      if (strncmp(variable_name, variables[i].name, MVAR_MAX_NAME_LENGTH) == 0) return i;
    #else
    char name_buf[MVAR_COMPOSITE_NAME_LENGTH + 1];
    uint16_t source_pos = 0;
    uint8_t len;
    for (uint8_t i = 0; i < num_variables; i++) {
      if (!get_next_word_from_contract(source_pos, len, name_buf, sizeof name_buf)) {
        return NO_VARIABLE;
      }
      remove_type(name_buf, len);
      if (strncmp(variable_name, name_buf, MVAR_MAX_NAME_LENGTH) == 0) return i;
    }
    #endif
    return NO_VARIABLE;
  }

  const ModuleVariable &get_module_variable(const uint8_t ix) const { return variables[ix]; }
  ModuleVariable &get_module_variable(const uint8_t ix) { return variables[ix]; }

  //const char *get_name(const uint8_t ix) const { return variables[ix].name; }
  ModuleVariableType get_type(const uint8_t ix) const { return variables[ix].get_type(); }

  // Low-level setter and getter. Use these on module size where ix is constant.
  void set_value(const uint8_t ix, const void *value, const uint8_t size) {
    if (ix < num_variables) variables[ix].set_value(value, size);
  }
  void get_value(const uint8_t ix, void *value, const uint8_t size) const {
    if (ix < num_variables) variables[ix].get_value(value, size);
  }
  
  const void *get_value_pointer(const uint8_t ix) const {
    static uint32_t zero_buffer = 0; // Used for returning a safe (zero) value when index out of range in get_value
    return ix < num_variables ? (void*)variables[ix].get_value_pointer() : (void*)&zero_buffer;
  }

  // Change-tolerant higher-level setter and getter. Use these on master side to handle changing contracts
  // when a module is reprogrammed while master is running.
  #ifdef USE_MIVARIABLE
  void verify_mivariable(MIVariable &var) const {
    // Search for the variable ix if first time or contract has changed
    if (var.ix == NO_VARIABLE || var.contract_id != contract_id || !contract_id) {
      var.ix = var.name != NULL ? get_variable_ix(var.name) : NO_VARIABLE;
      var.contract_id = var.ix != NO_VARIABLE ? contract_id : 0;
    }
  }
  void set_value(MIVariable &var, const void *value, const uint8_t size) {
    verify_mivariable(var);

    // Set the value with the verified ix
    if (var.ix < num_variables) variables[var.ix].set_value(value, size);
  }
  void get_value(MIVariable &var, void *value, const uint8_t size) const {
    verify_mivariable(var);

    // Get the value with the verified ix
    if (var.ix < num_variables) variables[var.ix].get_value(value, size);
  }
  const void *get_value_pointer(MIVariable &var) const {
    verify_mivariable(var);
    return get_value_pointer(var.ix);
  }

  // Specialized convenience setters
  void set_value(MIVariable &var, const bool &v) { set_value(var, &v, 1); }
  void set_value(MIVariable &var, const uint8_t &v) { set_value(var, &v, 1); }
  void set_value(MIVariable &var, const uint16_t &v) { set_value(var, &v, 2); }
  void set_value(MIVariable &var, const uint32_t &v) { set_value(var, &v, 4); }
  void set_value(MIVariable &var, const int8_t &v) { set_value(var, &v, 1); }
  void set_value(MIVariable &var, const int16_t &v) { set_value(var, &v, 2); }
  void set_value(MIVariable &var, const int32_t &v) { set_value(var, &v, 4); }
  void set_value(MIVariable &var, const float &v) { set_value(var, &v, 4); }

    // Specialized convenience getters
  void get_value(MIVariable &var, bool &v) const { get_value(var, &v, 1); }
  void get_value(MIVariable &var, uint8_t &v) const { get_value(var, &v, 1); }
  void get_value(MIVariable &var, uint16_t &v) const { get_value(var, &v, 2); }
  void get_value(MIVariable &var, uint32_t &v) const { get_value(var, &v, 4); }
  void get_value(MIVariable &var, int8_t &v) const { get_value(var, &v, 1); }
  void get_value(MIVariable &var, int16_t &v) const { get_value(var, &v, 2); }
  void get_value(MIVariable &var, int32_t &v) const { get_value(var, &v, 4); }
  void get_value(MIVariable &var, float &v) const { get_value(var, &v, 4); }

  // More specialized convenience getters
  bool     get_bool(MIVariable &var) const { return *(bool*)get_value_pointer(var); }
  uint8_t  get_uint8(MIVariable &var) const { return *(uint8_t*)get_value_pointer(var); }
  uint16_t get_uint16(MIVariable &var) const { return *(uint16_t*)get_value_pointer(var); }
  uint32_t get_uint32(MIVariable &var) const { return *(uint32_t*)get_value_pointer(var); }
  int8_t   get_int8(MIVariable &var) const { return *(int8_t*)get_value_pointer(var); }
  int16_t  get_int16(MIVariable &var) const { return *(int16_t*)get_value_pointer(var); }
  int32_t  get_int32(MIVariable &var) const { return *(int32_t*)get_value_pointer(var); }
  float    get_float(MIVariable &var) const { return *(float*)get_value_pointer(var); }

  // Change detection
  bool is_changed(MIVariable &var) const {
    verify_mivariable(var);
    if (var.ix < num_variables) return variables[var.ix].is_changed();
    return false;
  }

  // Event support
  bool is_event(MIVariable &var) const {
    verify_mivariable(var);
    if (var.ix < num_variables) return variables[var.ix].is_event();
    return false;
  }
  void set_event(MIVariable &var, const bool event = true) const {
    verify_mivariable(var);
    if (var.ix < num_variables) variables[var.ix].set_event(event);
  }
  #endif

  // Specialized convenience setters
  void set_value(const uint8_t ix, const bool &v) { set_value(ix, &v, 1); }
  void set_value(const uint8_t ix, const uint8_t &v) { set_value(ix, &v, 1); }
  void set_value(const uint8_t ix, const uint16_t &v) { set_value(ix, &v, 2); }
  void set_value(const uint8_t ix, const uint32_t &v) { set_value(ix, &v, 4); }
  void set_value(const uint8_t ix, const int8_t &v) { set_value(ix, &v, 1); }
  void set_value(const uint8_t ix, const int16_t &v) { set_value(ix, &v, 2); }
  void set_value(const uint8_t ix, const int32_t &v) { set_value(ix, &v, 4); }
  void set_value(const uint8_t ix, const float &v) { set_value(ix, &v, 4); }

  // Specialized convenience getters
  void get_value(const uint8_t ix, bool &v) const { get_value(ix, &v, 1); }
  void get_value(const uint8_t ix, uint8_t &v) const { get_value(ix, &v, 1); }
  void get_value(const uint8_t ix, uint16_t &v) const { get_value(ix, &v, 2); }
  void get_value(const uint8_t ix, uint32_t &v) const { get_value(ix, &v, 4); }
  void get_value(const uint8_t ix, int8_t &v) const { get_value(ix, &v, 1); }
  void get_value(const uint8_t ix, int16_t &v) const { get_value(ix, &v, 2); }
  void get_value(const uint8_t ix, int32_t &v) const { get_value(ix, &v, 4); }
  void get_value(const uint8_t ix, float &v) const { get_value(ix, &v, 1); }

  // More specialized convenience getters
  bool     get_bool(const uint8_t ix) const { return *(bool*)get_value_pointer(ix); }
  uint8_t  get_uint8(const uint8_t ix) const { return *(uint8_t*)get_value_pointer(ix); }
  uint16_t get_uint16(const uint8_t ix) const { return *(uint16_t*)get_value_pointer(ix); }
  uint32_t get_uint32(const uint8_t ix) const { return *(uint32_t*)get_value_pointer(ix); }
  int8_t   get_int8(const uint8_t ix) const { return *(int8_t*)get_value_pointer(ix); }
  int16_t  get_int16(const uint8_t ix) const { return *(int16_t*)get_value_pointer(ix); }
  int32_t  get_int32(const uint8_t ix) const { return *(int32_t*)get_value_pointer(ix); }
  float    get_float(const uint8_t ix) const { return *(float*)get_value_pointer(ix); }


  // Change detection
  bool is_changed() const {
    for (uint8_t i=0; i<num_variables; i++) if (variables[i].is_changed()) return true;
    return false;
  }
  void clear_changed() { for (uint8_t i=0; i<num_variables; i++) variables[i].set_changed(false); }
  bool is_changed(const uint8_t ix) const { if (ix < num_variables) return variables[ix].is_changed(); return false; }


  // Event support
  bool has_events() const {
    for (uint8_t i=0; i<num_variables; i++) if (variables[i].is_event()) return true;
    return false;
  }
  void clear_events() { for (uint8_t i=0; i<num_variables; i++) variables[i].set_event(false); }
  bool is_event(const uint8_t ix) const { if (ix < num_variables) return variables[ix].is_event(); return false; }
  void set_event(const uint8_t ix, const bool event = true) { if (ix < num_variables) variables[ix].set_event(event); }


  // These are used for determining when values are ready to be used. If setting values manually, call the set_updated
  // function when all values have been set so that they can be distributed to module or master.
  bool is_updated() const { return values_received_time != 0; }
  void set_updated() {
    if (!got_contract()) return;
    values_received_time = millis();
    if (values_received_time==0) values_received_time = 1;
  }
  uint32_t get_updated_time_ms() const { return values_received_time; }
  void clear_updated_if_too_old(uint32_t age_limit_ms = 3600000) {
    if ((uint32_t)(millis() - values_received_time) > age_limit_ms)  values_received_time = 0;
  }

  // Helper variables not used internally, available for use by a communication protocol
  #ifdef IS_MASTER
  uint32_t contract_requested_time = 0,
           requested_time = 0;
  #endif

  #if defined(DEBUG_PRINT) || defined(STATUS_PRINT) || defined(DEBUGPRINT_SETTINGS)
    void debug_print_contract() const {
      if (num_variables == 0) DPRINT(F("Empty contract."));
      else {
        DPRINT(num_variables); DPRINT(F(":"));
        char name_buf[MVAR_COMPOSITE_NAME_LENGTH + 1];
        uint16_t source_pos = 0;
        uint8_t len;
        for (uint8_t i = 0; i < num_variables; i++) {
          #ifdef IS_MASTER
          strncpy(name_buf, variables[i].name, MVAR_MAX_NAME_LENGTH);
          #else
          if (!get_next_word_from_contract(source_pos, len, name_buf, sizeof name_buf)) break;
          remove_type(name_buf, len);
          #endif
          if (i > 0) DPRINT(" ");
          DPRINT(name_buf); DPRINT(":");
          ModuleVariable::get_type_name(variables[i].get_type(), name_buf);
          DPRINT(name_buf);
        }
      }
      DPRINTLN("");
    }

    void debug_print_values() const {
      if (num_variables == 0) DPRINT(F("Empty contract."));
      else {
        char name_buf[MVAR_COMPOSITE_NAME_LENGTH + 1];
        uint16_t source_pos = 0;
        uint8_t len;
        for (uint8_t i = 0; i < num_variables; i++) {
          #ifdef IS_MASTER
          strncpy(name_buf, variables[i].name, MVAR_MAX_NAME_LENGTH);
          #else
          if (!get_next_word_from_contract(source_pos, len, name_buf, sizeof name_buf)) break;
          remove_type(name_buf, len);
          #endif
          if (i > 0) DPRINT(" ");
          DPRINT(name_buf); DPRINT("=");
          switch(variables[i].get_type()) {
          case mvtBoolean: DPRINT(*((bool*)variables[i].get_value_pointer())); break;
          case mvtUint8: DPRINT(*((uint8_t*)variables[i].get_value_pointer())); break;
          case mvtInt8: DPRINT(*((int8_t*)variables[i].get_value_pointer())); break;
          case mvtUint16: DPRINT(*((uint16_t*)variables[i].get_value_pointer())); break;
          case mvtInt16: DPRINT(*((int16_t*)variables[i].get_value_pointer())); break;
          case mvtUint32: DPRINT(*((uint32_t*)variables[i].get_value_pointer())); break;
          case mvtInt32: DPRINT(*((int32_t*)variables[i].get_value_pointer())); break;
          case mvtFloat32: DPRINT(*((float*)variables[i].get_value_pointer())); break;
          default: DPRINT(F("Unrecognized type ")); DPRINT(variables[i].get_type()); break;
          }
        }
      }
      DPRINTLN("");
    }
    #endif
};
