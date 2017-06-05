/* This sketch demonstrates the ModuleInterface library.
 * 
 * The sketch is a Master that is handling a group of modules/devices that have different tasks and 
 * different configurations. To add another module, just add it to the PJONModuleInterfaceSet
 * constructor paramter, separated by a space between each module name:id:bus.
 * 
 * It has the name and address of all relevant modules, and will contact each of them and get the
 * "contract" (details about settings, inputs and outputs) from it.
 * 
 * It will adjust the duty cycle setting of the BlinkModule, the setting will be transferred 
 * to the module automatically and the blink duty cycle will change visibly.
 * 
 * The output from the BlinkModule will be transferred as an output and will be printed by this
 * sketch to the serial line.
 * 
 * All is driven by this master sketch, with a default sampling interval of 10 seconds,
 * determined by the sampling_time* members of PJONMOduleInterfaceSet.
 */
 
#include <MiMaster.h>

// Modules
#define BLINKMODULE 0 // index of the BlinkModule interface if we need to access it directly

// PJON related
PJONLink<SoftwareBitBang> bus(1); // PJON device id 1

// Module interfaces
PJONModuleInterfaceSet interfaces(bus, "Blink:b1:4");

// We need a few selected measurements. The others we just transport.
MIVariable o_b1heartbeat("HeartB");
uint8_t last_heartbeat = 0;

// Settings (time interval and duty cycle)
MIVariable s_b1interval("TimeInt"), s_b1dutycycle("Duty");

uint8_t current_duty_cycle = 10;

void setup() {
  Serial.begin(115200);
  Serial.println("BlinkModuleMaster started.");

  bus.bus.strategy.set_pin(7);
  interfaces.set_receiver(receive_function);
  interfaces.set_notification_callback(notification_function);
  interfaces.sampling_time_outputs = 1000;
  
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
}

void loop() { 
  // Do data exchange to and from and between the modules
  interfaces.update();

  // Modify the BlinkModule duty cycle now and then
  set_settings();
  
  // Show activity LED
  static uint32_t last_led_change = millis();
  if (millis() - last_led_change >= 1000) {
    last_led_change = millis();
    static bool led_on = false;
    digitalWrite(13, led_on ? HIGH : LOW);
    led_on = !led_on;

    // Show inacitve modules (if module gets disconnected or dies)
    uint8_t inactive = interfaces.get_inactive_module_count();
    if (inactive > 0) {
      Serial.print(">>>>>> Inactive modules: "); Serial.println(inactive);
    }
  }
}

void notification_function(NotificationType notification_type, const ModuleInterface *module_interface) { 
  if (notification_type == ntNewOutputs) {
    // Display a measurement
    uint8_t heartbeat = interfaces[BLINKMODULE]->outputs.get_uint8(o_b1heartbeat);
    if (o_b1heartbeat.is_found()) {
      if (uint8_t(heartbeat - last_heartbeat) != 1) {
        Serial.print("############## Heartbeat not regular! Last="); Serial.print(last_heartbeat);
        Serial.print(", new="); Serial.println(heartbeat);
      } else { Serial.print("Heartbeat is "); Serial.println(heartbeat); }
      last_heartbeat = heartbeat;
    }
  } else if (notification_type == ntSampleSettings) {
    Serial.println("============ CHANGING DUTY CYCLE!");
    current_duty_cycle = 100 - current_duty_cycle;
    interfaces[BLINKMODULE]->settings.set_value(s_b1dutycycle, current_duty_cycle);
  }
}

void receive_function(const uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info, const ModuleInterface *module_interface){
  // Handle specialized messages to this module
  Serial.print("!!!!!!!!!!!!!!!!! CUSTOM MESSAGE from ");
  Serial.print(module_interface ? module_interface->module_name : "unregistered device");
  Serial.print(", len "); Serial.print(length); Serial.print(":");
  for (int i=0; i<length; i++) { Serial.print(payload[i]); Serial.print(" "); }
  Serial.println();
}

void set_settings() {
  if (interfaces[BLINKMODULE]->settings.got_contract() && !interfaces[BLINKMODULE]->settings.is_updated()) {
    uint32_t interval = 2000;
    interfaces[BLINKMODULE]->settings.set_value(s_b1interval, interval);
    interfaces[BLINKMODULE]->settings.set_value(s_b1dutycycle, current_duty_cycle);
    interfaces[BLINKMODULE]->settings.set_updated(); // Ready to use
  }
}

