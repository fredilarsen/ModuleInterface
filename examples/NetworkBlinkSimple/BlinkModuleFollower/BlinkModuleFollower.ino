 /* This sketch demonstrates the ModuleInterface library.
 * 
 * It does not use any settings, but receive an online updated input value from the BlinkModule which is
 * given the prefix "b1" by the BlinkModuleMasterMultiple. Input variable names are prefixed with the 
 * lower-case two-character module prefix where the values originate.
 * This input measures the blink interval of BlinkModule, and this module follows it by using the
 * same interval.
 */

#include <MIModule.h>

// Settings
#define iTimeIntervalIx 0

// PJON related
uint8_t bus_id[4] = {3,3,3,4};
PJONLink<SoftwareBitBang> bus(bus_id, 5); // PJON device id 5

PJONModuleInterface interface("BlinkF",                          // Module name
                              bus,                                // PJON bus
                              "",                                 // Settings
                              "b1NowInt:u4",                      // Inputs                       
                              "");                                // Outputs (measurements)                         

void setup() {
  bus.bus.strategy.set_pin(7);
  
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  interface.update();
  blink();
}

void blink() {
  // Get the configured interval when received from Master
  uint32_t interval = 30; // Blink rapidly until a valid interval received from master
  if (interface.inputs.is_updated()) interface.inputs.get_value(iTimeIntervalIx, interval);
  
  // Turn on or off when interval elapsed
  static uint32_t last_change = millis();
  static bool light_on = false;
  uint32_t now = millis();
  if (now - last_change >= interval) {
    last_change = now;
    light_on = !light_on;
    digitalWrite(LED_BUILTIN, light_on ? HIGH : LOW);
  }
}

