 /* This sketch turns light on and off based on a time schedule and by comparing an external ambient light measurement 
    to a configured limit.
    So if time is within the configured time-of-day interval _and_ the measured light is below the limit, light is kept on.
 */
//#define DEBUG_PRINT
#include <MIModule.h>
#include <utils/MITime.h>

// Settings
#define s_mode_ix                0
#define s_light_limit_ix         1
#define s_time_interval_start_ix 2
#define s_time_interval_end_ix   3

// Inputs
#define i_ambientlight_ix 0

// Outputs
#define o_lighton_ix 0
#define o_utc_ix     1

// PJON related
PJONLink<SoftwareBitBang> link(20); // PJON device id 20

PJONModuleInterface interface("LightCon",                             // Module name
                              link,                                   // PJON bus
                              "Mode:u1 Limit:u2 TStartM:u2 TEndM:u2", // Settings
                              "smLight:u2",                           // Inputs                       
                              "LightOn:u1 UTC:u4");                   // Outputs (measurements)                         

// Allow user to change the mode with a button locally on this module, this will be synced back to web page
const uint8_t MODESWITCH_PIN = 4;                              

void setup() {
  Serial.begin(115200);
  Serial.println("LightController example v1.2");
    
  link.bus.strategy.set_pin(7);
    
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  
  pinMode(MODESWITCH_PIN, INPUT_PULLUP);
}

void loop() {
  interface.update();
  lightcontrol();
  check_user_input();
}

void lightcontrol() {
  // Require that all settings, inputs and time sync are updated before starting
  if (interface.settings.is_updated() && interface.inputs.is_updated() && interface.is_time_set()) {

    // Check if within configured time-of-day interval (or if no interval is defined)
    uint16_t minute_of_day = miGetMinuteOfDay(interface.get_time_utc_s()),
             start_minute_of_day = interface.settings.get_uint16(s_time_interval_start_ix),
             end_minute_of_day = interface.settings.get_uint16(s_time_interval_end_ix);
    if (start_minute_of_day > end_minute_of_day) end_minute_of_day += 24*60;
    if (start_minute_of_day > minute_of_day) minute_of_day += 24*60;
    bool within_interval = (start_minute_of_day == end_minute_of_day) 
      || (start_minute_of_day <= minute_of_day && minute_of_day <= end_minute_of_day);

    // Check light level
    bool below_limit = interface.inputs.get_uint16(i_ambientlight_ix) <= 
                       interface.settings.get_uint16(s_light_limit_ix);

    // Turn light on or off
    uint8_t mode = interface.settings.get_uint16(s_mode_ix);  // Get the controller mode (on/off/auto)
    bool light_on = mode == 1 || (mode==2 && within_interval && below_limit);
    digitalWrite(LED_BUILTIN, light_on ? HIGH : LOW);
    
    // Report status back
    interface.outputs.set_value(o_lighton_ix, light_on);
    interface.outputs.set_value(o_utc_ix, interface.get_time_utc_s());
    interface.outputs.set_updated();  
  }
}

void check_user_input() {
  // Check if user changes mode locally (connect pin MODESWITCH_PIN to ground momentarily)
  static bool modePinLowLast = false;
  bool modePinLow = digitalRead(MODESWITCH_PIN) == LOW;
  if (modePinLow && !modePinLowLast) {
    uint8_t mode = interface.settings.get_uint8(s_mode_ix);
    mode = ++mode % 3;
    interface.settings.set_value(s_mode_ix, mode);
    Serial.print("Mode set to "); Serial.println(mode);
  }
  modePinLowLast = modePinLow;
}