#pragma once

#include <ModuleInterface.h>
#include <EEPROM.h>

// Persistent storage
const uint32_t MAGICNUMBER_MI_EEPROM = 2786000001; // Change this number when changing storage format/variables

uint16_t eeprom_read_bytes(void *buf, uint16_t length, uint16_t eeprom_pos) {
  uint8_t *p = (uint8_t*) buf;
  uint16_t i;
  for (i = 0; i < length; i++) *p++ = EEPROM.read(eeprom_pos++);
  return i;
}

uint16_t eeprom_write_bytes(const void *buf, uint16_t length, uint16_t eeprom_pos) {
  const uint8_t *p = (const uint8_t*) buf;
  uint16_t i;
  for (i = 0; i < length; i++) EEPROM.write(eeprom_pos++, *p++);
  return i;
}

// Write the bytes to EEPROM only if not matching what is already there. This saves write operations and
// prolongues the life of the EEPROM. If content has not changed it caused no wear at all.
uint16_t eeprom_update_bytes(const void *buf, uint16_t length, uint16_t eeprom_pos, bool &modified) {
  const uint8_t *p = (const uint8_t*) buf;
  uint16_t i;
  for (i = 0; i < length; i++) {
    if (*p != EEPROM.read(eeprom_pos)) { EEPROM.write(eeprom_pos, *p); modified = true; }
    *p++;
    eeprom_pos++;
  }
  return i;
}

bool write_settings_to_eeprom(const ModuleInterface &mi, uint16_t eeprom_start_pos = 0) {
  // Make sure it is properly filled in before storing
  if (!mi.settings.got_contract() || !mi.settings.is_updated()) return false;
  
  // Store a magic number to be able to read reliably back
  bool modified = false;
  uint16_t p = eeprom_start_pos;
  p += eeprom_update_bytes(&MAGICNUMBER_MI_EEPROM, sizeof MAGICNUMBER_MI_EEPROM, p, modified); // Magic number
  uint32_t contract_id = mi.settings.get_contract_id(); 
  p += eeprom_update_bytes(&contract_id, sizeof contract_id, p, modified); // Store contract id early, it is essential when reading back
  
  // Get serialized settings contract and store it (EDIT: Is it needed? If contract id is present values can be checked against it)
  BinaryBuffer buf;
  uint8_t length = 0;
//  mi.settings.get_variables(BinaryBuffer &buf, uint8_t &length, mcSetSettingContract);
//  eeprom_update_bytes(length, sizeof length); p += sizeof length; // Store length explicitly
//  eeprom_update_bytes(buf.get(), length); p+= length;
  
  // Get serialized settings values and store it
  length = 0;
  mi.settings.get_values(buf, length, mcSetSettings);
  p += eeprom_update_bytes(&length, sizeof length, p, modified); // Store length explicitly  
  p += eeprom_update_bytes(buf.get(), length, p, modified);
  #ifdef DEBUG_PRINT
    Serial.print(modified ? "----> Wrote bytes to EEPROM: " : "No need to update EEPROM with bytes: "); Serial.println(length);
  #endif
  return true;
}

bool write_to_eeprom_when_needed(ModuleInterface &mi, uint32_t &last_save, uint32_t save_interval_ms = 600000, uint16_t eeprom_start_pos = 0) {
  // Only save settings when actually changed and not too often (do not wear out EEPROM memory)
  if (millis() - last_save >= save_interval_ms) {
    last_save = millis();
    return write_settings_to_eeprom(mi, eeprom_start_pos);
  }
  return false; // Not saved
}

bool read_settings_from_eeprom(ModuleInterface &mi, uint16_t eeprom_start_pos = 0) {
  // Make sure it has an updated contract before reading
  if (!mi.settings.got_contract()) return false;  
  
  // Read magic number
  uint16_t p = eeprom_start_pos;
  uint32_t number = 0;
  p += eeprom_read_bytes(&number, sizeof number, p); // Magic number
  if (number != MAGICNUMBER_MI_EEPROM) return false; // Not ModuleInterface serialized data, so return
 
  // Read contract id
  uint32_t contract_id = 0;
  p += eeprom_read_bytes(&contract_id, sizeof contract_id, p); // Magic number
  if (contract_id != mi.settings.get_contract_id() || contract_id == 0) return false; // Not the correct contract, so return
 
  // Read and deserialize settings contract
  // (EDIT: Is there any need?)
  
  // Read and deserialize settings values
  uint8_t length = 0;
  p += eeprom_read_bytes(&length, sizeof length, p);
  uint8_t buf[length];
  p += eeprom_read_bytes(buf, length, p);
  #ifdef DEBUG_PRINT
    Serial.print("Read bytes from EEPROM: "); Serial.println(p-eeprom_start_pos); 
  #endif  
  bool status = mi.settings.set_values(buf+1, length-1); // Skip header byte
  return status;
}

