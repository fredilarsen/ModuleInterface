#pragma once

#include <MI/ModuleInterface.h>
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

  // Get (import) settings from external source to be distributed to modules
  virtual void get_settings() = 0;

  // Put (export) settings from modules to external target
  virtual void put_settings() = 0;

  // Put (export) values from modules to external target
  virtual void put_values() = 0;
};
