/* This sketch demonstrates the ModuleInterface library.

   The sketch simply reads a motion detection sensor, and exposes it as an output.
   More sensors can be added, just add them to the outputs list to have them transferred.
   Data types: b=bool, u=unsigned integer, i=signed integer, f=float. Followed by byte count (1,2 or 4).

   The ModuleInterface enables sensor readings to be distributed to other modules and logged in 
   a database, for example for plotting in a web page or triggering of events.
*/

#include <MiModule.h>

PJONLink<SoftwareBitBang> link(4); // PJON device id 4

PJONModuleInterface interface("SensMon",                          // Module name
                              link,                               // PJON bus
                              "",                                 // Settings
                              "",                                 // Inputs
                              "Motion:b1");                       // Outputs (measurements)

// Outputs (measurements) (index/position in outputs list)
#define o_motion_ix 0

#define PIN_MOTIONSENSOR 6 // Motion sensor connected to this pin

void setup() { pinMode(PIN_MOTIONSENSOR, INPUT); link.bus.strategy.set_pin(7); }

void loop() { read_sensors(); interface.update(); }

void read_sensors() {
  interface.outputs.set_value(o_motion_ix, digitalRead(PIN_MOTIONSENSOR));
  interface.outputs.set_updated(); // Flag as completely updated, all values are set
}

