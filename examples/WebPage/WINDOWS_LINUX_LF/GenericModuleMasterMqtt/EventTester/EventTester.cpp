/* This is a test program for using events to and from a MQTT broker.
   An event is an extraordinary prioritized transfer of one or more values,
   flagged to be transferred earlier than the first scheduled transfer.
*/

#define PJON_INCLUDE_LF
#define MI_USE_SYSTEMTIME
#define DEBUG_PRINT_TIMES
//#define DEBUG_PRINT_SETTINGSYNC
//#define MIMQTT_USE_JSON

#ifdef _WIN32
  // MS compiler does not like PJON_MAX_PACKETS=0 in PJON
  #define PJON_MAX_PACKETS 1
#endif

#include <MIModule.h>
#include <utils/MITime.h>
#include <utils/MIUtilities.h>

// PJON related
PJONLink<LocalFile> bus(20); // PJON device id 20

PJONModuleInterface interface("LightCon", // Module name
  bus,                                    // PJON bus
  "Target:f4",                            // Settings
  "ETemp:f4",                             // Inputs                       
  "Temp:f4 TTarg:f4 UTC:u4 ETempOut:f4"); // Outputs (measurements)                         

// Settings
#define s_target_ix       0

// Inputs
#define i_externaltemp_ix 0

// Outputs
#define o_temp_ix         0
#define o_target_ix       1
#define o_utc_ix          2
#define o_externaltemp_ix 3

uint32_t last_out_event_time = 0, last_setting_event_time = 0, last_control_time = 0;

float target = 20, temp = 20;


void lightcontrol() {
  // Require that all settings, inputs and time sync are updated before starting
  if (interface.settings.is_updated() && interface.is_time_set()) {
/*
    if (mi_interval_elapsed(last_out_event_time, 8000)) {
      temp += 3.0;
      interface.outputs.set_value(o_temp_ix, temp);
      interface.outputs.set_event(o_temp_ix); // Flag it for immediate transfer
      printf("Increase temp output to %g\n", temp);
    }
*/
/*
    // Set target every now and then from the device
    if (mi_interval_elapsed(last_setting_event_time, 17000)) {
      target = 20;
      interface.settings.set_value(s_target_ix, target);
      interface.settings.set_event(s_target_ix); // Flag it for immediate transfer
      printf("Set target setting to %g\n", target);
    }
*/
    // Step controller towards target temp every second
    if (mi_interval_elapsed(last_control_time, 1000)) {
      if (abs(target - temp) > 0.001f) {
        temp += (target - temp) / 50.0f;
        interface.outputs.set_value(o_temp_ix, temp);
        interface.outputs.set_event(o_temp_ix);
      }
    }
  }
}

void notification_function(NotificationType notification_type, const ModuleInterface *) {
  if (notification_type == ntSampleOutputs) {
    printf("Sampling outputs\n");
    interface.outputs.set_value(o_temp_ix, temp);
    interface.outputs.set_value(o_target_ix, target);
    interface.outputs.set_value(o_utc_ix, interface.get_time_utc_s());
    interface.outputs.set_value(o_externaltemp_ix, interface.inputs.get_float(i_externaltemp_ix));
    interface.outputs.set_updated();
  }
  else if (notification_type == ntNewInputs) {
    // Show an input value
    float exttemp = 0;
    interface.inputs.get_value(i_externaltemp_ix, exttemp);
    if (interface.inputs.get_module_variable(i_externaltemp_ix).is_event())
      printf("Incoming input EVENT, exttemp is %f\n", exttemp); 
    else
      printf("Incoming timer-based input, exttemp is %f\n", exttemp);
  }
  else if (notification_type == ntNewSettings) {
    // New settings
    target = interface.settings.get_float(s_target_ix);
    if (interface.settings.get_module_variable(s_target_ix).is_event()) {
      printf("Incoming setting EVENT, target = %g\n", target);
    } else
      printf("Incoming timer-based setting, target = %g\n", target);
  }
}

void setup(int argc, const char * const argv[]) {
  printf("Welcome to EventTester (LocalFile).\n");

  // Now get the rest of the configuration from the web server
  bus.bus.begin();
  interface.set_notification_callback(notification_function);
}

void loop() {
  interface.update();
  lightcontrol();
  delay(1);
}

int main(int argc, const char * const argv[]) {
  setup(argc, argv);
  while (true) loop();
  return 0;
}
