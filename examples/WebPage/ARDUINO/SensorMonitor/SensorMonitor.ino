/* This ModuleInterface example sketch simply reads and reports some sensor values */

#include <MiModule.h>

PJONLink<SoftwareBitBang> link(10); // PJON device id 10

PJONModuleInterface interface("Outdoor",                          // Module name
                              link,                               // PJON bus
                              "",                                 // Settings
                              "",                                 // Inputs
                              "Light:u2 Motion:u1");              // Outputs (measurements)

// Outputs (measurements) (index/position in outputs list)
#define o_light_ix  0
#define o_motion_ix 1

#define PIN_LIGHTSENSOR  A0 // Analog light sensor connected to this pin
#define PIN_MOTIONSENSOR 6  // Motion sensor connected to this pin

void setup() { 
  pinMode(PIN_LIGHTSENSOR, INPUT);
  pinMode(PIN_MOTIONSENSOR, INPUT); 
  link.bus.strategy.set_pin(7);
}

void loop() { read_sensors(); interface.update(); }

void read_sensors() {
  interface.outputs.set_value(o_light_ix, analogRead(PIN_LIGHTSENSOR));
  interface.outputs.set_value(o_motion_ix, digitalRead(PIN_MOTIONSENSOR));
  interface.outputs.set_updated(); // Flag as completely updated, all values are set
}