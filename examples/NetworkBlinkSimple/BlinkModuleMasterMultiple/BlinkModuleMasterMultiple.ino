/* This sketch demonstrates the ModuleInterface library used in network mode, which means that it
 * is using PJON bus ids and support devices spread on multiple buses, available through routers
 * or as in this case, shared media with a common SWBB wire.
 * 
 * Connect it like the standard BlinkModuleMasterMultiple.
 */

#define USE_MIVARIABLE // For named access to variables, constant even if contract changes

#include <MIMaster.h>

// Modules
#define BLINKMODULE  0 // index of the BlinkModule interface if we need to access it directly
#define BLINKMODULE2 1 // index of the BlinkModule2 interface if we need to access it directly

// PJON related
uint8_t bus_id[4] = {3,3,3,3};
PJONLink<SoftwareBitBang> bus(bus_id, 1); // PJON device id 1

// Module interfaces (name:prefix:PJON device id:PJON bus id)
PJONModuleInterfaceSet interfaces(bus, "Blink:b1:4:3.3.3.3 Blink2:b2:5:3.3.3.4");

// A variable for accessing the interval (could be accessed by position as well)
MIVariable s_interval("TimeInt");

void setup() {
  bus.bus.strategy.set_pin(7);
  interfaces.sampling_time_settings = 3000; // Transfer settings a little faster than default
  interfaces.sampling_time_outputs = 3000;  // Transfer outputs and inputs faster too
}

void loop() { 
  interfaces.update(); // Do data exchange to and from and between the modules
  set_settings();      // Modify the BlinkModule interval every now and then
}

void set_settings() {
  // Change the blink interval regularly
  static uint32_t interval = 0;
  static uint32_t last_change = 30;
  if (millis() - last_change > 10000) {
    last_change = millis();
    interval = interval < 500 ? 500 : 100;
  }

  // Set the interval as a setting for the Blink module
  interfaces[BLINKMODULE]->settings.set_value(s_interval, interval);
  interfaces[BLINKMODULE]->settings.set_updated(); // Flag that all settings are ready to use
}