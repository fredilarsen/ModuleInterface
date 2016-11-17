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
  char name[MVAR_MAX_NAME_LENGTH + 1];
  uint8_t value[4];  // A 4 byte buffer covers all supported value types, better than using a pointer and allocating memory
  ModuleVariableType type = mvtUnknown;

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
    const char *pos = strstr(s, ":");
    if (pos != NULL) {
      uint8_t len = min(pos - s, MVAR_MAX_NAME_LENGTH);
      memcpy(name, s, len);
      name[len] = 0; // Null-terminate
      type = get_type(&pos[1]);
    } else { // Type not specified, use float as default type
      uint8_t len = min(strlen(s), MVAR_MAX_NAME_LENGTH);
      memcpy(name, s, len);
      name[len] = 0; // Null-terminator
      type = mvtFloat32; // Default data type
    }        
  }
  
  // Return whether this variable name has a module prefix (lower case) or is a local name
  bool has_module_prefix() const {
    return name[0] >= 'a' && name[0] <= 'z';
  }
  // Return prefixed name, either prefixed from before, or with a prefix added now
  void get_prefixed_name(const char *prefix, char *output_name_buf) const {
    if (has_module_prefix() || !prefix) strcpy(output_name_buf, name); // Already prefixed
    else { // Add the specified prefix
      uint8_t len = strlen(prefix);
      strncpy(output_name_buf, prefix, len);
      strncpy(&output_name_buf[len], name, MVAR_MAX_NAME_LENGTH - len);
      output_name_buf[MVAR_MAX_NAME_LENGTH] = 0;
    }
  }
  
  // Value setters and getters
  void set_value(const void *v, const uint8_t size) { if (get_size() == size) memcpy(value, v, size); }
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
    switch(type) {
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



