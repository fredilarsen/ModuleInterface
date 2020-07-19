/* This sketch demonstrates the ModuleInterface library.

   The sketch does a simple task that requires some configuration. That configuration is kept in a
   ModuleInterface object, allowing it to be set and read from another device through some protocol,
   in this case PJON.
   The remote device will upon connection get this device's contract, that is, the names and types of
   the settings and inputs it requires, and the outputs it offers.

   The sketch takes a measurement as well, and this is transported to the remote side as an output.
   It does not require or demonstrate any inputs.

   This sketch illustrates saving of settings to EPROM for continued self-sufficient operation with
   the same settings after a restart.

   It also illustrates how to add a custom handler for PJON messages. PJON messages can be sent and
   received as usual, even with the ModuleInterface communication doing its work.

   Also shown is the use of the notification callback. This allows immediate action to be taken when
   new settings or inputs arrive, or to do sampling immediately before outputs are retrieved. It is
   important not to do long-lasting work in the notification function for ntSampleOutputs, as it
   will delay the master.
*/

// Save some RAM by reducing PJON packet size, and by including only the strategy in use
#define PJON_PACKET_MAX_LENGTH 50
#define PJON_INCLUDE_SWBB

#include <MIModule.h>
#include <PJONSoftwareBitBang.h>

// Settings
#define s_time_interval_ix 0
#define s_dutycycle_ix     1

// Inputs
#define i_return_heartbeat_ix 0

// Outputs (measurements)
#define o_uptime_ix    0
#define o_heartbeat_ix 1

// Illustrating the use of PROGMEM to save RAM (especially useful if there are many parameters)
const char PROGMEM settings[] = "TimeInt:u4 Duty:u1";
const char PROGMEM inputs[]   = "HeartB:u1";
const char PROGMEM outputs[]  = "Uptime:u4 HeartB:u1";

PJONLink<SoftwareBitBang> bus(4); // PJON device id 4
PJONModuleInterface interface("Blink", bus, true, settings, inputs, outputs);

bool light_on = false;
uint8_t heartbeat = 0;

void setup() {
  Serial.begin(115200);
  Serial.println(F("BlinkModule started."));

  bus.bus.set_receiver(receive_function);
  bus.bus.strategy.set_pin(7);
  interface.set_notification_callback(notification_function);
  read_settings_from_eeprom(interface);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  interface.update();
  blink();
}

void notification_function(NotificationType notification_type, const ModuleInterface *) {
  if (notification_type == ntSampleOutputs) {
    interface.outputs.set_value(o_heartbeat_ix, ++heartbeat); // Set heartbeat
    interface.outputs.set_value(o_uptime_ix, millis()); // Set uptime
    interface.outputs.set_updated(); // Flag as completely updated
  }
  else if (notification_type == ntNewInputs) {
    // Show an input value
    uint8_t heartbeat_return = 0;
    interface.inputs.get_value(i_return_heartbeat_ix, heartbeat_return);
    Serial.print(F("Return heartbeat is ")); Serial.println(heartbeat_return);
  }
  else if (notification_type == ntNewSettings) {    
    // Make sure settings are kept in EEPROM, ready for self-sufficient work after reboot
    static uint32_t last_eeprom_save = 0;
    write_to_eeprom_when_needed(interface, last_eeprom_save);
  }
}

void receive_function(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info) {
  // Let interface handle incoming messages and reply if needed
  if (interface.handle_message(payload, length, packet_info)) return;

  // Handle specialized messages to this module, or broadcast
  if (do_something(payload, length)) return;

  // Else unrecognized message
}

bool do_something(uint8_t * message, uint16_t length) {
  Serial.print("do_something received "); Serial.print(message[0]);
  Serial.print(" len "); Serial.println(length);
  return false;
}

void blink() {
  // Use configured interval and duty cycle and current state to find actual interval
  // (keep on for duty cycle * interval and off for (1-duty cycle)*interval)
  uint32_t interval = 30;
  if (interface.settings.is_updated()) {
    interface.settings.get_value(s_time_interval_ix, interval);
    uint8_t dutycycle_percent = interface.settings.get_uint8(s_dutycycle_ix);
    interval = light_on ? dutycycle_percent * interval / 100 : (100 - dutycycle_percent) * interval / 100;
  }

  // Turn on or off when interval elapsed
  static uint32_t last_change = millis();
  uint32_t now = millis();
  if (now - last_change >= interval) {
    last_change = now;
    light_on = !light_on;
    digitalWrite(LED_BUILTIN, light_on ? HIGH : LOW);
  }
}

