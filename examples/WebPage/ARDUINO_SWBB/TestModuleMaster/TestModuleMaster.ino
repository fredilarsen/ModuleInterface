/* This is a test master for the simplified ModuleInterface example setup which includes:
 * 1. A master (Nano, Uno, ...)
 * 2. A module reading the ambient light level (Nano, Uno, ...)
 * 3. A module which turns the light on and off (on-board LED for now) (Nano, Uno, ...)
 * 
 * On-board LED blinks:
 * - Every second when both worker modules are active and all is well
 * - Fast if no activity from one or both modules
 * - Very fast if a runtime error in master module
 */
//#define DEBUG_PRINT
#define USE_MIVARIABLE
#include <MIMaster.h>
#include <PJONSoftwareBitBang.h>

// PJON related
PJONLink<SoftwareBitBang> bus(1); // PJON device id 1

// Module interfaces
PJONModuleInterfaceSet interfaces(bus, "SensMon:sm:10 LightCon:lc:20", "m1");

// Some selected values to print
MIVariable o_smLight("Light"), o_lcLightOn("LightOn");

// Module settings to set
MIVariable s_lcMode("Mode"), s_lcLimit("Limit"), s_lcTStartM("TStartM"), s_lcTEndM("TEndM");

void setup() {
  Serial.begin(115200);
  Serial.println("TestModuleMaster (SWBB) started.");
  bus.bus.strategy.set_pin(7);
  bus.bus.begin();
  
  // Set frequency of transfer between modules
  interfaces.set_transfer_interval(2000); // Transfer a little faster than default
  
  // Register a callback function for printing incoming values
  interfaces.set_notification_callback(notification_function);

  // Init on-board LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {  
  interfaces.update();    // Data exchange to and from and between the modules
  flash_status_led();     // Show module status by flashing LED
  set_modulesettings();
}

void flash_status_led() {
  // Let activity flash go to rapid if one or more modules are inactive, faster if low mem
  static uint32_t last_led_change = millis();
  uint16_t intervalms = interfaces.get_inactive_module_count() > 0 ? 300 : 1000;
  if (mvs_out_of_memory) intervalms = 30;
  if (mi_interval_elapsed(last_led_change, intervalms)) {
    static bool led_on = false;
    digitalWrite(LED_BUILTIN, led_on ? HIGH : LOW);
    led_on = !led_on;
  }
}

void notification_function(NotificationType notification_type, const ModuleInterface *interface) {
  if (notification_type == ntNewOutputs) {
    // Display measured light from SensorMonitor
    uint16_t measured_light = interface->outputs.get_uint16(o_smLight);
    if (o_smLight.is_found()) { Serial.print("Measured light level: "); Serial.println(measured_light); }

    // Display current LED status in LightController
    uint8_t led_light = interface->outputs.get_uint8(o_lcLightOn);
    if (o_lcLightOn.is_found()) { Serial.print("Output LED light: "); Serial.println(led_light); }
  }
}

void set_modulesettings() {
  static bool did_set_settings = false;
  if (interfaces.got_all_contracts() && !did_set_settings) {
    did_set_settings = true; // No need to repeat setting these static values

    miTime::Set(1529764437); // We have no time source, so start with a valid fixed time

    ModuleVariableSet *settings = interfaces.find_settings_by_prefix("sm");
    if (settings) settings->set_updated();

    settings = interfaces.find_settings_by_prefix("lc");
    if (settings) {
      settings->set_value(s_lcMode, (uint8_t)2); // Auto
      settings->set_value(s_lcMode, (uint8_t)2); // Auto
      settings->set_value(s_lcLimit, (uint16_t)200);
      settings->set_value(s_lcTStartM, (uint16_t)0);
      settings->set_value(s_lcTEndM, (uint16_t)0);
      settings->set_updated();
    }
  }
}
