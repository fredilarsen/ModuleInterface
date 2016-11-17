/* This sketch demonstrates the ModuleInterface library.
 * 
 * It sets a time interval to be used on the BlinkModule sketch, and that setting is synchronized
 * to two instances of that sketch.
 * The interval is changed every 5s here, and that should be visible in the other devices.
 * 
 * It can also handle one BlinkModule (id 4) and one BlinkModuleFollower (id 5). In this case,
 * only the BlinkModule receives the interval setting. The BlinkModuleFollower has no settings
 * but has the interval output from the BlinkModule as an input. This demonstrates the online exchange
 * of values between modules (through the master).
 */

#include <PJONModuleInterfaceSet.h>
#include <PJONLink.h>

// Modules
#define BLINKMODULE  0 // index of the BlinkModule interface if we need to access it directly
#define BLINKMODULE2 1 // index of the BlinkModule2 interface if we need to access it directly

// PJON related
PJONLink<SoftwareBitBang> bus(1); // PJON device id 1

// Module interfaces (name:prefix:PJON device id)
PJONModuleInterfaceSet interfaces(bus, "Blink:b1:4 Blink2:b2:5");

// A variable for accessing the interval (could be accessed by position as well)
MIVariable s_interval("TimeInt"), s_interval2("TimeInt");

void setup() {
  bus.bus.strategy.set_pin(7);
  interfaces.sampling_time_settings = 1000; // Sync settings every second, a little faster than default
  interfaces.sampling_time_outputs = 1000; // Sync outputs and inputs every second
}

void loop() { 
  interfaces.update(); // Do data exchange to and from and between the modules
  set_settings();      // Modify the BlinkModule interval every now and then
}

void set_settings() {
  // Change the blink interval every 5 seconds
  static uint32_t interval = 0;
  static uint32_t last_change = 30;
  if (millis() - last_change > 5000) {
    last_change = millis();
    interval = interval < 500 ? 500 : 100;
  }

  // Set the same interval on both modules' interfaces
  if (interfaces[BLINKMODULE]->settings.got_contract()) {
    interfaces[BLINKMODULE]->settings.set_value(s_interval, interval);
    interfaces[BLINKMODULE]->settings.set_updated(); // Flag that all settings are ready to use
  }
  if (interfaces[BLINKMODULE2]->settings.got_contract()) { // BlinkModuleFollower will ignore this
    interfaces[BLINKMODULE2]->settings.set_value(s_interval2, interval);
    interfaces[BLINKMODULE2]->settings.set_updated(); // Flag that all settings are ready to use
  }
}

