#include <platforms/MISystemDefines.h>

// Short name for each of the enums (2 characters each)
const char ModuleVariableTypeNames[] PROGMEM = "--b1u1u2u4i1i2i4f4";

const char * const get_mv_type_names() { return ModuleVariableTypeNames; }  