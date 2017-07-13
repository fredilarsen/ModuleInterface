#include <MI/ModuleVariable.h>

// Short name for each of the enums (2 characters each)
const char ModuleVariableTypeNames[] PROGMEM = "--b1u1u2u4i1i2i4f4";

ModuleVariableType ModuleVariable::get_type(const char *type_name) {
  uint8_t len = sizeof ModuleVariableTypeNames / 2;
  char name[3];
  for (uint8_t i = 0; i < len; i++) {
    name[0] = pgm_read_byte(&(ModuleVariableTypeNames[i*2]));
    name[1] = pgm_read_byte(&(ModuleVariableTypeNames[i*2 + 1]));
    name[2] = 0;
    if (strncmp(type_name, name, 2) == 0) return (ModuleVariableType) i;
  }
  return mvtUnknown;
}

void ModuleVariable::get_type_name(const ModuleVariableType mvt, char name[]) {
  uint8_t pos = 2* (uint8_t) mvt;
  name[0] = pgm_read_byte(&(ModuleVariableTypeNames[pos]));
  name[1] = pgm_read_byte(&(ModuleVariableTypeNames[pos + 1]));
  name[2] = 0;
}
