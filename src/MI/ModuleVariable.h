#pragma once

#include <platforms/MIPlatforms.h>
#include <utils/BinaryBuffer.h>

// To avoid memory fragmentation the name buffers are preallocated.
// The maximum name length can be overridden by defining MVAR_MAX_NAME_LENGTH before including this file.
// Variable names always start with an upper case letter, unless prefixed with a lower case module prefix.
#ifndef MVAR_MAX_NAME_LENGTH
  #define MVAR_MAX_NAME_LENGTH 10
#endif

// The first part of a variable can be a lower case module prefix.
// (This is included in the MVAR_MAX_NAME_LENGTH)
#ifndef MVAR_PREFIX_LENGTH
  #define MVAR_PREFIX_LENGTH 2
#endif

// Type length including colon (":u4" etc)
#define MVAR_TYPE_LENGTH 3

// Name including prefix, plus type length
#define MVAR_COMPOSITE_NAME_LENGTH (MVAR_MAX_NAME_LENGTH + MVAR_TYPE_LENGTH)

// Suppress strncpy warnings
#ifdef MI_POSIX
#pragma warning(disable:4996)
#endif

enum ModuleVariableType {
  mvtUnknown,
  mvtBoolean,
  mvtUint8,
  mvtUint16,
  mvtUint32,
  mvtInt8,
  mvtInt16,
  mvtInt32,
  mvtFloat32
};

extern const char * const get_mv_type_names();

struct ModuleVariable {
private:
  union {
    uint32_t uint32; // To make sure it is properly aligned
    float f;
    char array[4];   // A 4 byte buffer covers all supported value types, better than using a pointer and allocating memory
  } value;
  ModuleVariableType type = mvtUnknown; // To save memory we use the uppermost bits for change detection, therefore do not allow direct access
public:
  #ifdef IS_MASTER
  char name[MVAR_MAX_NAME_LENGTH + 1];
  #ifdef MASTER_MULTI_TRANSFER
  // Used for managing reverse settings to multiple (max 7) transfers at the same time (HTTP, MQTT, ...)
  uint8_t change_bits = 0;
  bool get_change_bit(uint8_t bit) const { return (change_bits & (1 << bit)) > 0; }
  void clear_change_bit(uint8_t bit) { change_bits &= ~(1 << bit); }
  void set_change_bits() { change_bits = 0xFF; }
  bool any_change_bit(uint8_t bitcount) const {
    for (uint8_t t = 0; t < bitcount; t++) if (get_change_bit(t)) return true;
    return false;
  }
  // The 8th bit, bit 7, is reserved to detect if the value has ever been set after startup.
  // (If set from a source like MQTT that transports one value at a time, this flag can be checked
  // to determine if a whole ModuleVariableSet has been set or if only some variables have been set.)
  void set_initialized() { change_bits |= (1 << 7); }
  bool is_initialized() const { return (change_bits & (1 << 7)) != 0; }
  #endif
  #endif

  ModuleVariable() {
    #ifdef IS_MASTER
    name[0] = 0;
    #endif
    memset(value.array, 0, 4);
  }

  #ifdef IS_MASTER
  ModuleVariable(const ModuleVariable &source) {
    memcpy(name, source.name, sizeof name);
    memcpy(value.array, source.value.array, 4);
    memcpy(&type, &source.type, sizeof type);
  }
  #endif

  // Setters and getters for serializing (type,length,name)
  void set_variable(const uint8_t *name_and_type) {
    // Read name length byte
    type = (ModuleVariableType) name_and_type[0];
    #ifdef IS_MASTER
    uint8_t len = (uint8_t) MI_min(name_and_type[1], MVAR_MAX_NAME_LENGTH);
    memcpy(name, &name_and_type[2], len);
    name[len] = 0; // Null-terminator
    #endif
  }

  // Setting from text
  void set_variable(const char *s) { // Format like "LightOn:b1"
    // Read name length byte
    const char *pos1 = strchr(s, ':'), *pos2 = strchr(s, ' ');
    if (pos1 == NULL || (pos2 != NULL && pos2 < pos1)) { // No colon in this variable declaration, use float as default
      #ifdef IS_MASTER
      uint8_t len = (uint8_t) MI_min(pos2 == NULL ? strlen(s) : pos2-s, MVAR_MAX_NAME_LENGTH);
      memcpy(name, s, len);
      name[len] = 0; // Null-terminator
      #endif
      type = mvtFloat32; // Default data type
    } else { // There is a colon in the declaration
      #ifdef IS_MASTER
      uint8_t len = (uint8_t) MI_min(pos1 - s, MVAR_MAX_NAME_LENGTH);
      memcpy(name, s, len);
      name[len] = 0; // Null-terminate
      #endif
      type = get_type(&pos1[1]);
    }
  }

  #ifdef IS_MASTER
  // Return whether this variable name has a module prefix (lower case) or is a local name
  bool has_module_prefix() const { return name[0] >= 'a' && name[0] <= 'z'; }

  // Return prefixed name, either prefixed from before, or with a prefix added now
  void get_prefixed_name(const char *prefix, char *output_name_buf, uint8_t buf_size) const {
    if (has_module_prefix() || !prefix) strncpy(output_name_buf, name, buf_size); // Already prefixed
    else { // Add the specified prefix
      uint8_t len = (uint8_t) strlen(prefix);
      strncpy(output_name_buf, prefix, MI_min(len, buf_size));
      strncpy(&output_name_buf[len], name, buf_size - len);
      output_name_buf[buf_size-1] = 0;
    }
  }
  #endif

  ModuleVariableType get_type() const { return (ModuleVariableType) (type & 0b00111111); }

  // Change detection
  void set_changed(bool changed = true) {
    if (changed) type = (ModuleVariableType)(type | 0b10000000);
    else type = (ModuleVariableType)(type & 0b01111111);
  }
  bool is_changed() const { return (type & 0b10000000) != 0; }

  // Event flag
  void set_event(bool event = true) {
    if (event) type = (ModuleVariableType)(type | 0b01000000);
    else type = (ModuleVariableType)(type & 0b10111111);
  }
  bool is_event() const { return (type & 0b01000000) != 0; }

  // Value setters and getters
  void set_value(const void *v, const uint8_t size) {
    if (get_size() == size) {
      if (memcmp(value.array, v, size) != 0) set_changed(true);
      memcpy(value.array, v, size);
      #if defined(IS_MASTER) && defined(MASTER_MULTI_TRANSFER)
      set_initialized();
      #endif
    }
  }
  void get_value(void *v, const uint8_t size) const { if (get_size() == size) memcpy(v, value.array, size); }

  bool is_equal(const void *v, const uint8_t size) { 
    if (get_size() == size) {
      if (get_type() == mvtFloat32 && isfinite(value.f) && isfinite(*(float*)v)) {
        return (fabs(value.f - *(const float*)v) < 1e-5*fabs(value.f));
      } else return (memcmp(value.array, v, size) == 0); 
    }
    return false;
  }
  
  const void *get_value_pointer() const { return value.array; }
  void *get_value_pointer() { return value.array; }
  
  // Specialized convenience setters (these do not cost memory because of inlining)
  void set_value(const bool v) { set_value(&v, 1); }
  void set_value(const uint8_t v) { set_value(&v, 1); }
  void set_value(const uint16_t v) { set_value(&v, 2); }
  void set_value(const uint32_t v) { set_value(&v, 4); }
  void set_value(const int8_t v) { set_value(&v, 1); }
  void set_value(const int16_t v) { set_value(&v, 2); }
  void set_value(const int32_t v) { set_value(&v, 4); }
  void set_value(const float v) { set_value(&v, 4); }

  // Specialized convenience getters (these do not cost memory because of inlining)
  void get_value(bool &v) const { get_value(&v, 1); }
  void get_value(uint8_t &v) const { get_value(&v, 1); }
  void get_value(uint16_t &v) const { get_value(&v, 2); }
  void get_value(uint32_t &v) const { get_value(&v, 4); }
  void get_value(int8_t &v) const { get_value(&v, 1); }
  void get_value(int16_t &v) const { get_value(&v, 2); }
  void get_value(int32_t &v) const { get_value(&v, 4); }
  void get_value(float &v) const { get_value(&v, 1); }

  // More specialized convenience getters (these do not cost memory because of inlining)
  bool get_bool() const { return *(bool*) get_value_pointer(); }
  uint8_t get_uint8() const {  return *(uint8_t*) get_value_pointer(); }
  uint16_t get_uint16() const {  return *(uint16_t*) get_value_pointer(); }
  uint32_t get_uint32() const { return value.uint32; }
  int8_t get_int8() const {  return *(int8_t*) get_value_pointer(); }
  int16_t get_int16() const {  return *(int16_t*) get_value_pointer(); }
  int32_t get_int32() const {  return *(int32_t*) get_value_pointer(); }
  float get_float() const {  return *(float*) get_value_pointer(); }

  bool get_value_as_text(char *text, uint8_t maxlen) const {
    if (!text || maxlen < 10) return false;
    switch(get_type()) {
    case mvtBoolean: strcpy(text, get_bool() ? "true" : "false"); return true;
    case mvtUint8: sprintf(text, "%d", get_uint8()); return true;
    case mvtInt8: sprintf(text, "%d", get_int8()); return true;
    case mvtUint16: sprintf(text, "%d", get_uint16()); return true;
    case mvtInt16: sprintf(text, "%d", get_int16()); return true;
    case mvtUint32: sprintf(text, "%ld", get_uint32()); return true;
    case mvtInt32: sprintf(text, "%ld", get_int32()); return true;
    case mvtFloat32: sprintf(text, "%f", get_float()); return true;
    case mvtUnknown: return false;
    }
    return false;
  }

  bool set_value_from_text(const char *text) {
    if (!text) return false;
    switch(get_type()) {
    case mvtBoolean: set_value(text[0]=='1' || text[0]=='t' || text[0]=='T'); return true;
    case mvtUint8: set_value((uint8_t)atoi(text)); return true;
    case mvtInt8: set_value((int8_t)atoi(text)); return true;
    case mvtUint16: set_value((uint16_t)atoi(text)); return true;
    case mvtInt16: set_value((int16_t)atoi(text)); return true;
    case mvtUint32: set_value((uint32_t)atol(text)); return true;
    case mvtInt32: set_value((int32_t)atol(text)); return true;
    case mvtFloat32: set_value((float)atof(text)); return true;
    case mvtUnknown: return false;
    }
    return false;
  }

  // Memory management
  uint8_t get_size() const {
    switch(get_type()) {
    case mvtBoolean:
    case mvtUint8:
    case mvtInt8: return 1;
    case mvtUint16:
    case mvtInt16: return 2;
    case mvtUint32:
    case mvtInt32:
    case mvtFloat32: return 4;
    case mvtUnknown: return 0;
    }
    return 0;
  }
  
  static ModuleVariableType get_type(const char *type_name) {
    uint8_t len = (uint8_t) strlen(get_mv_type_names()) / 2;
    char name[3];
    for (uint8_t i = 0; i < len; i++) {
      name[0] = pgm_read_byte(&(get_mv_type_names()[i*2]));
      name[1] = pgm_read_byte(&(get_mv_type_names()[i*2 + 1]));
      name[2] = 0;
      if (strncmp(type_name, name, 2) == 0) return (ModuleVariableType) i;
    }
    return mvtUnknown;
  }

  static void get_type_name(const ModuleVariableType mvt, char name[]) {
    uint8_t pos = 2* (uint8_t) mvt;
    name[0] = pgm_read_byte(&(get_mv_type_names()[pos]));
    name[1] = pgm_read_byte(&(get_mv_type_names()[pos + 1]));
    name[2] = 0;
  }
};
