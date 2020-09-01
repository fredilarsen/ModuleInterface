#include <MiniMI.h>

const uint8_t RELAY_PIN = 4, TEMP_PIN = A1, MOIST_PIN = A2, PRESS_PIN = A3;

MiniMI<SoftwareBitBang> mi(44, F("Relay:u1"), F("Temp:u2 Moist:u2 Press:u2"));

void begin() {
  pinMode(RELAY_PIN, OUTPUT_PULLUP);
  pinMode(TEMP_PIN, INPUT);
  pinMode(MOIST_PIN, INPUT);
  pinMode(PRESS_PIN, INPUT);
  mi.begin(7);
}

void loop() {
  mi.loop();
  measure();
  relay();
}

void measure() {
  // Read sensor values
  mi.set_output(0, analogRead(TEMP_PIN));
  mi.set_output(1, analogRead(MOIST_PIN));
  mi.set_output(2, analogRead(PRESS_PIN)));
  mi.set_outputs_updated();
}

void relay() {
  // Operate relay
  if (mi.settings_updated())
    digitalWrite(RELAY_PIN, mi.get_setting_uint8(0) ? LOW : HIGH);
}