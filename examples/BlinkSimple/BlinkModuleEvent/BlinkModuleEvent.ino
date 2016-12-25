 /* This sketch demonstrates the ModuleInterface library.
 * 
 * This sketch demonstrates immediate (event) transfer of an output to an input.
 * It is meant to be used in a setup with BlinkModuleFollower and BlinkModuleMasterMultiple.
 * 
 * This sketch is identical to BlinkModule except for that is looks for a negative flank
 * on digital pin 8, changing the output variable when the pin is connected to ground, and flagging 
 * this as an event to request immediate transfer without waiting for the regular sampling.
 * The result is that the BlinkFollower module will change its blink interval immediately.
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
  
  pinMode(8, INPUT_PULLUP);
}

void loop() {
  interface.update();
  blink();
  check_pin();
}

void blink() {
  // Get the configured interval when received from Master
  uint32_t interval = 30; // Blink rapidly until a valid interval received from master
  if (interface.settings.is_updated()) interface.settings.get_value(s_time_interval_ix, interval);

  // Send the interval back as an output
  static uint32_t last_interval = 0;
  if (last_interval != interval) {
    interface.outputs.set_value(o_time_interval_ix, interval);
    interface.outputs.set_updated();
  }
  last_interval = interval;  
  
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

// Check if pin 8 has been connected to earth
void check_pin() {
  bool pin_low = digitalRead(8) == LOW;
  static bool previous_pin_low = true;
  if (pin_low && !previous_pin_low) { // Negative flank, pin was connected to earth
    interface.outputs.set_value(o_time_interval_ix, (uint32_t) 20); // Rapid blink
    interface.outputs.set_event(o_time_interval_ix); // Flag it for immediate transfer
  }
  previous_pin_low = pin_low;
}
