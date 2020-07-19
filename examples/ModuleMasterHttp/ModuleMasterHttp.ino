/* This sketch demonstrates the ModuleInterface library.
 * 
 * The sketch is a Master that is handling a group of modules/devices that have different tasks and 
 * different configurations. To add another module, just add it to the PJONModuleInterfaceSet
 * constructor parameter, separated by a space between each module name:prefix:id:bus.
 * 
 * It has the name and address of all relevant modules, and will contact each of them and get the
 * "contract" (details about settings, inputs and outputs) from it.
 * 
 * It will use a web client to retrieve the settings for all the modules from a database behind a
 * web server (typically a Apache+MariaDb/MySQL+PHP setup) and distribute these to the modules so
 * that they know what to do. A module can be passive until it retrieves its configuration, or
 * keep the configuration in EPROM. A function is available for easy EPROM persistence.
 * Whenever a setting in the database is changed by a user through a configuration web page,
 * the setting will be transferred to the relevant module automatically.
 * 
 * Outputs (if any) from each modules will also be retrieved automatically, and these will be
 * available (mirrored) on the master in the interface objects. They can be located by name
 * by the master if it needs any of them.
 * Outputs (usually measurements, but can also be calculated values) will be archived in the database
 * by the same web client, making it possible to plot historical data in a web page, and do 
 * post-processing or analysis.
 * 
 * If any of the modules have any inputs, the outputs from the other modules will be searched,
 * and a matching output will be mapped to the input. 
 * In this way, a sensor measurement from one module can be distributed automatically to all other 
 * modules that may need it.
 * 
 * All is driven by this master sketch, with a default sampling interval of 10 seconds,
 * determined by the sampling_time* members of PJONMOduleInterfaceSet.
 * 
 * This sketch must run on an Arduino Mega or similar because of high memory usage caused by all the
 * libraries that are used.
 */

#define MI_HTTPCLIENT
#include <MIMaster.h>
#include <PJONSoftwareBitBang.h>

// PJON related
PJONLink<SoftwareBitBang> bus(1); // PJON device id 1

// Web server related
uint8_t web_server[] = {192,1,1,71};
EthernetClient web_client;

// Ethernet configuration for this device
byte gateway[] = { 192, 1, 1, 1 };
byte subnet[] = { 255, 255, 255, 0 };
byte mac[] = {0xDE, 0xCD, 0x4E, 0xEF, 0xFE, 0xED};
byte ip[] = { 192, 1, 1, 180 };

// Module interfaces
PJONModuleInterfaceSet interfaces(bus, "Blink:b1:4", "m1");
// TODO: This could even be retrieved from the web server at startup, to make it possible to
// configure and activate new modules in a web page :-)
MIHttpTransfer http_transfer(interfaces, web_client, web_server);

void setup() {
  Ethernet.begin(mac, ip, gateway, gateway, subnet);

  Serial.begin(9600);
  Serial.println(F("ModuleMasterHttp started."));

  bus.bus.strategy.set_pin(7);
  bus.bus.begin();

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);  
}

void loop() {
  interfaces.update(&http_transfer); // Do all data exchange
  flash_status_led(); // Show module status by flashing LED
}

void flash_status_led() {  
  // Let activity flash go to rapid if one or more modules are inactive, faster if low mem
  static uint32_t last_led_change = millis();
  uint16_t intervalms = interfaces.get_inactive_module_count() > 0 ? 300 : 1000;
  if (mvs_out_of_memory) intervalms = 30;
  if (millis() - last_led_change >= intervalms) {
    last_led_change = millis();
    static bool led_on = false;
    digitalWrite(LED_BUILTIN, led_on ? HIGH : LOW);
    led_on = !led_on;
  }
}



