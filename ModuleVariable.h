#pragma once

#include <BinaryBuffer.h>

// To avoid memory fragmentation the name buffers are preallocated.
// The maximum name length can be overridden by defining MVAR_MAX_NAME_LENGTH before including this file.
// Variable names always start with an upper case letter, unless prefixed with a lower case module prefix.
#ifndef MVAR_MAX_NAME_LENGTH
  #define MVAR_MAX_NAME_LENGTH 10
#endif

// The first part of a variable can be a lower case module prefix.
#ifndef MVAR_PREFIX_LENGTH
  #define MVAR_PREFIX_LENGTH 2
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

// Short name for each of the enums
extern const char *ModuleVariableTypeNames[];

struct ModuleVariable {
private:
  ModuleVariableType type = mvtUnknown; // To save memory we use the uppermost bits for change detection, therefore do not allow direct access
public:  
  uint8_t value[4];  // A 4 byte buffer covers all supported value types, better than using a pointer and allocating memory
  char name[MVAR_MAX_NAME_LENGTH + 1];

  ModuleVariable() { name[0] = 0; memset(value, 0, 4); }
  
  // Setters and getters for serializing (type,length,name)
  void set_variable(const uint8_t *name_and_type, const uint8_t length) {
    // Read name length byte
    type = (ModuleVariableType) name_and_type[0];
    uint8_t len = min(name_and_type[1], MVAR_MAX_NAME_LENGTH);
    memcpy(name, &name_and_type[2], len);
    name[len] = 0; // Null-terminator   
  }

  // Setting from text
  void set_variable(const char *s) { // Format like "lightOn:b1"   
    // Read name length byte
    const char *pos1 = strchr(s, ':'), *pos2 = strchr(s, ' ');
    if (pos1 == NULL || (pos2 != NULL && pos2 < pos1)) { // No colon in this variable declaration, use float as default
      uint8_t len = min(pos2 == NULL ? strlen(s) : pos2-s, MVAR_MAX_NAME_LENGTH);
      memcpy(name, s, len);
      name[len] = 0; // Null-terminator
      type = mvtFloat32; // Default data type
    } else { // There is a colon in the declaration
      uint8_t len = min(pos1 - s, MVAR_MAX_NAME_LENGTH);
      memcpy(name, s, len);
      name[len] = 0; // Null-terminate
      type = get_type(&pos1[1]);
    }
  }
  
  // Return whether this variable name has a module prefix (lower case) or is a local name
  bool has_module_prefix() const {
    return name[0] >= 'a' && name[0] <= 'z';
  }
  // Return prefixed name, either prefixed from before, or with a prefix added now
  void get_prefixed_name(const char *prefix, char *output_name_buf, uint8_t buf_size) const {
    if (has_module_prefix() || !prefix) strncpy(output_name_buf, name, buf_size); // Already prefixed
    else { // Add the specified prefix
      uint8_t len = strlen(prefix);
      strncpy(output_name_buf, prefix, min(len, buf_size));
      strncpy(&output_name_buf[len], name, buf_size - len);
      output_name_buf[buf_size-1] = 0;
    }
  }

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
      if (memcmp(value, v, size) != 0) set_changed(true); // Detect changes
      memcpy(value, v, size);
    }
  }
  void get_value(void *v, const uint8_t size) const { if (get_size() == size) memcpy(v, value, size); }

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
  bool get_bool() const { return *(bool*)value; }
  uint8_t get_uint8() const { return *(uint8_t*)value; }
  uint16_t get_uint16() const { return *(uint16_t*)value; }
  uint32_t get_uint32() const { return *(uint32_t*)value; }
  int8_t get_int8() const { return *(int8_t*)value; }
  int16_t get_int16() const { return *(int16_t*)value; }
  int32_t get_int32() const { return *(int32_t*)value; }
  float get_float() const { return *(float*)value; }  
  
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
    }
    return 0;
  }
  static ModuleVariableType get_type(const char *type_name) {
    for (uint8_t i = 0; ModuleVariableTypeNames[i] != NULL; i++) {
      if (strncmp(type_name, ModuleVariableTypeNames[i], strlen(ModuleVariableTypeNames[i])) == 0)
        return (ModuleVariableType) i;
    }
    return mvtUnknown;
  }
};



