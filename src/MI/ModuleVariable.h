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
  #endif

  ModuleVariable() {
    #ifdef IS_MASTER
    name[0] = 0;
    #endif
    memset(value.array, 0, 4);
  }

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
  void set_variable(const char *s) { // Format like "lightOn:b1"
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
  static ModuleVariableType get_type(const char *type_name);
  static void get_type_name(const ModuleVariableType mvt, char name[]); // name must have length >= MVAR_TYPE_LENGTH chars
};
