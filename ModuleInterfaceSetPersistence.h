#pragma once

#include <ModuleInterfaceSet.h>
#include <ModuleInterfacePersistence.h>

// Persistent storage
const uint32_t MAGICNUMBER_MIS_EEPROM = 1757080001; // Change this number when changing storage format/variables

void write_to_eeprom_when_needed(ModuleInterfaceSet *mis, uint32_t &last_save, uint32_t save_interval_ms = 600000) {
  // Only save settings when actually changed and not too often (do not wear out EEPROM memory)
  if (millis() - last_save >= 600000)) {
    last_save = millis();
    if (last_save > 0) write_settings_to_eeprom(mis); // Do not save right after startup
  }
}

void write_settings_to_eeprom(const ModuleInterfaceSet &mis, uint16_t eprom_start_pos = 0) {
  // magic number
  
  // Number of interfaces
  
  // Write each interface
  for (int i=0; i < mis.num_interfaces; i++) write_settings_to_eeprom(mis.interfaces[i]);
}

void read_settings_from_eeprom(ModuleInterfaceSet &mis, uint16_t eprom_start_pos = 0) {
  // Read magic number
  
  // Read number of interfaces
  
  // Read each interface
}
