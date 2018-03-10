#include <MI/ModuleInterface.h>

void dummy_notification_function(NotificationType /*notification_type*/, const ModuleInterface* /*module_interface*/) {};

#ifndef IS_MASTER
const char *ModuleInterface::settings_contract = NULL, // Pointer to ordinary or PROGMEM string constant
           *ModuleInterface::inputs_contract = NULL,
           *ModuleInterface::outputs_contract = NULL;

// Callbacks for reading contracts from ordinary string constants
char ModuleInterface::settings_callback(uint16_t pos) { return settings_contract[pos]; }
char ModuleInterface::inputs_callback(uint16_t pos) { return inputs_contract[pos]; }
char ModuleInterface::outputs_callback(uint16_t pos) { return outputs_contract[pos]; }

// Callbacks for reading contracts from PROGMEM string constants
char ModuleInterface::settings_callback_P(uint16_t pos) { return pgm_read_byte(&(settings_contract[pos])); }
char ModuleInterface::inputs_callback_P(uint16_t pos) { return pgm_read_byte(&(inputs_contract[pos])); }
char ModuleInterface::outputs_callback_P(uint16_t pos) { return pgm_read_byte(&(outputs_contract[pos])); }  
#endif

