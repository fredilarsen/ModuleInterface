/* This sketch demonstrates the ModuleInterface library.
 * 
 * It sets a time interval to be used on the BlinkModule sketch, and that setting is synchronized
 * to that device. The interval is changed every 5s here, and that should be visible in the other device.
 */

#define USE_MIVARIABLE // For named access to variables, constant even if contract changes
 
#include <MIMaster.h>
#include <PJONSoftwareBitBang.h>

// Modules
#define BLINKMODULE 0 // index of the BlinkModule interface if we need to access it directly

// PJON related
PJONLink<SoftwareBitBang> bus(1); // PJON device id 1

// Module interfaces (name:prefix:PJON device id)
PJONModuleInterfaceSet interfaces(bus, "Blink:b1:4");

// A variable for accessing the interval setting
MIVariable s_interval("TimeInt");

void setup() {
  bus.bus.strategy.set_pin(7);
  interfaces.set_transfer_interval(1000); // Sync every second, a little faster than default
}

void loop() { 
  interfaces.update(); // Do data exchange to and from and between the modules
  set_settings();      // Modify the BlinkModule interval every now and then
}

void set_settings() {
  if (interfaces[BLINKMODULE]->settings.got_contract()) {
    static uint32_t last_change = 0;
    static uint32_t interval = 0;
    if (millis() - last_change > 5000) { // Change blink interval every 5 seconds
      last_change = millis();
      interval = interval < 500 ? 500 : 100;
      interfaces[BLINKMODULE]->settings.set_value(s_interval, interval);
      interfaces[BLINKMODULE]->settings.set_updated(); // Flag that settings are ready to use  
    }
  }
}

