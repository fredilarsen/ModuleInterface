#pragma once

#include <MI/ModuleInterfaceSet.h>
#include <utils/MITime.h>

#define NUM_SCAN_INTERVALS 4

struct MILastScanTimes {
  uint32_t times[NUM_SCAN_INTERVALS];

  // Also keep track of time usage for web requests
  uint32_t last_get_settings_usage_ms = 0,
           last_set_settings_usage_ms = 0,
           last_set_values_usage_ms = 0;

  MILastScanTimes() {
    memset(times, 0, NUM_SCAN_INTERVALS*sizeof(uint32_t));
  }
};


class MITransferBase {
protected:  
  // Configuration
  ModuleInterfaceSet &interfaces;

  // State
  uint32_t last_settings = 0, last_outputs = millis();
  MILastScanTimes last_scan_times;

public:
  MITransferBase(ModuleInterfaceSet &module_interface_set) : 
                 interfaces(module_interface_set) {
  }

  #ifdef MASTER_MULTI_TRANSFER
  uint8_t transfer_ix = 0; // Which changed-bit to use
  static uint8_t transfer_count;
  #endif

  static void clear_mv_changed(ModuleVariable &mv, uint8_t transfer_ix) {
    #ifdef MASTER_MULTI_TRANSFER
    mv.clear_change_bit(transfer_ix); // Clear only this transfer's changed-bit
    #else
    mv.set_changed(false);
    #endif
  }

  static bool is_mv_changed(const ModuleVariable &mv, uint8_t transfer_ix) {
    #ifdef MASTER_MULTI_TRANSFER
    return mv.get_change_bit(transfer_ix); // Get only this transfer's changed-bit
    #else
    return mv.is_changed();
    #endif
  }

  static bool is_mvs_changed(const ModuleVariableSet &mvs, uint8_t transfer_ix) {
    for (uint8_t i = 0; i < mvs.get_num_variables(); i++)
      if (is_mv_changed(mvs.get_module_variable(i), transfer_ix)) return true;
    return false;
  }

  static void set_mv_and_changed_flags(ModuleVariable &mv, const void *value, uint8_t size, uint8_t transfer_ix) {
    if (mv.is_changed()) {
      // Discard changes from database if changes from module are present and not registered yet.
      // Reset changed-flag if value from database matches the current value, meaning that the change
      // from the module has been transported to the database and back.
      #ifdef DEBUG_PRINT
      DPRINT("Changed, equal="); DPRINT(mv.is_equal(value, size)); DPRINT(", val="); DPRINTLN(*(const float*)value);
      #endif
      if (mv.is_equal(value, size)) clear_mv_changed(mv, transfer_ix);
    }
    else {
      mv.set_value(value, size);
      #ifdef MASTER_MULTI_TRANSFER
      // Set changed-bit for other transfers so that they get a copy
      // (clear_mv_changed further down will clear the bit for this transfer)
      if (mv.is_changed() && !mv.any_change_bit(transfer_count)) mv.set_change_bits();
      #endif
      clear_mv_changed(mv, transfer_ix); // Do not set changed-flag when set from database, only in the opposite direction
    }
  }

  // Poll or maintain connection repeatedly
  virtual void update() = 0;

  // Get (import) settings from external source to be distributed to modules
  virtual void get_settings() = 0;

  // Put (export) settings from modules to external target
  virtual void put_settings() = 0;

  // Get (import) values from external target to module inputs
  virtual void get_values() = 0;

  // Put (export) values from module outputs to external target
  virtual void put_values() = 0;
};
