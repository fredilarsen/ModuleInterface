/* This is a simplified master for the ModuleInterface example setup which includes:
* 1. A master (Linux or Windows) with NO connection to a web server / database
* 2. A LUDP<->SWBB switch (Nano or Uno with Ethernet shield and BlinkingRGBSwitch PJON example)
* 3. A module reading the ambient light level (Nano, Uno, ...)
* 4. A module which turns the light on and off (on-board LED for now) (Nano, Uno, ...)
*/

//#define DEBUG_PRINT
#define PJON_INCLUDE_LUDP
#define USE_MIVARIABLE
#define MI_USE_SYSTEMTIME

#ifdef _WIN32
  // MS compiler does not like PJON_MAX_PACKETS=0 in PJON
  #define PJON_MAX_PACKETS 1
#endif

#include <MIMaster.h>

// PJON related
PJONLink<LocalUDP> bus(1); // PJON device id 1

// Module interfaces
PJONModuleInterfaceSet interfaces(bus, "SensMon:sm:10 LightCon:lc:20", "m1");

// Some selected values to print
MIVariable o_smLight("Light"), o_lcLightOn("LightOn");

// Module settings to set
MIVariable s_lcMode("Mode"), s_lcLimit("Limit"), s_lcTStartM("TStartM"), s_lcTEndM("TEndM");

void notification_function(NotificationType notification_type, const ModuleInterface *interface) {
  if (notification_type == ntNewOutputs) {
    // Display measured light from SensorMonitor
    uint16_t measured_light = interface->outputs.get_uint16(o_smLight);
    if (o_smLight.is_found()) printf("Measured light level: %d\n", measured_light);

    // Display current LED status in LightController
    uint8_t led_light = interface->outputs.get_uint8(o_lcLightOn);
    if (o_lcLightOn.is_found()) printf("Output LED light: %d\n", led_light);
  }
}

void set_modulesettings() {
  static bool did_set_settings = false;
  if (interfaces.got_all_contracts() && !did_set_settings) {
    did_set_settings = true; // No need to repeat setting these static values

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

void setup() {
  printf("Welcome to TestModuleMasterHttp (LUDP).\n");
  bus.bus.strategy.set_port(7200); // Must be the same for all devices
  bus.bus.begin();

  // Set frequency of transfer between modules
  interfaces.sampling_time_settings = 2000;
  interfaces.sampling_time_outputs = 2000;
  
  // Register a callback function for printing incoming values
  interfaces.set_notification_callback(notification_function);
}

void loop() {
  interfaces.update();    // Data exchange to and from and between the modules
  set_modulesettings();
  delay(10);
}

int main() {
  setup();
  while (true) loop();
  return 0;
}
