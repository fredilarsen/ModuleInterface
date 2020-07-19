/* This ModuleInterface example sketch simply reads and reports some sensor values */

#include <MIModule.h>
#include <PJONSoftwareBitBang.h>

PJONLink<SoftwareBitBang> link(10); // PJON device id 10

PJONModuleInterface interface("Outdoor",                          // Module name
                              link,                               // PJON bus
                              "",                                 // Settings
                              "",                                 // Inputs
                              "Light:u2 LightLP:f4 Motion:u1");   // Outputs (measurements)

// Outputs (measurements) (index/position in outputs list)
#define o_light_ix        0
#define o_lowpasslight_ix 1
#define o_motion_ix       2

#define PIN_LIGHTSENSOR  A0 // Analog light sensor connected to this pin
#define PIN_MOTIONSENSOR 6  // Motion sensor connected to this pin

void setup() { 
  pinMode(PIN_LIGHTSENSOR, INPUT);
  pinMode(PIN_MOTIONSENSOR, INPUT); 
  link.bus.strategy.set_pin(7);
}

void loop() { read_sensors(); interface.update(); }

void read_sensors() {
  static uint32_t last_time = millis();
  if (mi_interval_elapsed(last_time, 100)) {
    // Read light sensor, and maintain a low pass filtered value
    uint16_t light = 1023 - analogRead(PIN_LIGHTSENSOR);
    static float lowpasslight = light;
    lowpasslight = mi_lowpass(light, lowpasslight, 0.001f);
  
    // Register the outputs
    interface.outputs.set_value(o_light_ix, light);
    interface.outputs.set_value(o_lowpasslight_ix, lowpasslight);
    interface.outputs.set_value(o_motion_ix, digitalRead(PIN_MOTIONSENSOR));
    interface.outputs.set_updated(); // Flag as completely updated, all values are set
  }
}