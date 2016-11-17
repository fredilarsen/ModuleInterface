 /* This sketch demonstrates the ModuleInterface library.
 * 
 * It does a simple task that requires some configuration. That configuration is kept in a 
 * ModuleInterface object, allowing it to be set and read from another device through some protocol,
 * in this case PJON.
 * The Master device will upon connection get this device's contract, that is, the names and types of
 * the settings and inputs it requires, and the outputs it offers.
 * 
 * This sketch demonstrates synchronization of a setting and an output. The output is transferred but
 * not used except in a setup with BlinkModuleMasterMultiple and BlinkModuleFollower.
 */

#include <PJONModuleInterface.h>
#include <PJONLink.h>

// Settings
#define s_time_interval_ix 0

// Outputs
#define o_time_interval_ix 0

// PJON related
PJONLink<SoftwareBitBang> bus(4); // PJON device id 4 (change it to 5 for instance 2 of BlinkModule)

PJONModuleInterface interface("Blink",                            // Module name
                              bus,                                // PJON bus
                              "TimeInt:u4",                       // Settings
                              "",                                 // Inputs                       
                              "NowInt:u4");                       // Outputs (measurements)                         

void setup() {
  bus.bus.strategy.set_pin(7);
  
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
}

void loop() {
  interface.update();
  blink();
}

void blink() {
  // Get the configured interval when received from Master
  uint32_t interval = 30; // Blink rapidly until a valid interval received from master
  if (interface.settings.is_updated()) interface.settings.get_value(s_time_interval_ix, interval);

  // Send the interval back as an output
  interface.outputs.set_value(o_time_interval_ix, interval);
  interface.outputs.set_updated();
  
  // Turn on or off when interval elapsed
  static uint32_t last_change = millis();
  static bool light_on = false;
  uint32_t now = millis();
  if (now - last_change >= interval) {
    last_change = now;
    light_on = !light_on;
    digitalWrite(13, light_on ? HIGH : LOW);
  }
}

