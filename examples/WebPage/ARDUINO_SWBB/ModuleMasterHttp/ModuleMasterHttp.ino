/* This is the master for the full ModuleInterface example setup which includes:
 * 1. A master (Arduino Mega plus Ethernet shield)
 * 2. A module reading the ambient light level (Nano, Uno, ...)
 * 3. A module which turns the light on and off (on-board LED for now) (Nano, Uno, ...)
 * 4. A webserver+database setup which keeps all the module configuration and logs their output
 * 5. A web site that shows the status of the modules, allows their configuration to be changed,
 *    and to view their output as instant values or in trend plots.
 */

#include <MiMaster.h>

// PJON related
PJONLink<SoftwareBitBang> bus(1); // PJON device id 1

// Web server related
IPAddress web_server_ip(192,1,1,143);
EthernetClient web_client;

// Ethernet configuration for this device
uint8_t gateway[] = { 192, 1, 1, 1 };
uint8_t subnet[] = { 255, 255, 255, 0 };
uint8_t mac[] = {0xDE, 0xCD, 0x4E, 0xEE, 0xEE, 0xED};
uint8_t ip[] = { 192, 1, 1, 181 };

// Module interfaces
PJONModuleInterfaceSet interfaces(bus, "SensMon:sm:10 LightCon:lc:20", "m1");
MIHttpTransfer http_transfer(interfaces, web_client, web_server_ip, 1000, 1000);

void setup() {
  Ethernet.begin(mac, ip, gateway, gateway, subnet);
  
  bus.bus.strategy.set_pin(7);
  bus.bus.begin();
  
  // Set frequency of transfer between modules
  interfaces.sampling_time_settings = 1000;
  interfaces.sampling_time_outputs = 1000;
  
  // Init on-board LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {  
  interfaces.update();    // Data exchange to and from and between the modules
  http_transfer.update(); // Data exchange to and from web server    
  flash_status_led();     // Show module status by flashing LED
}

void flash_status_led() {
  // Let activity flash go to rapid if one or more modules are inactive, faster if low mem
  static uint32_t last_led_change = millis();
  uint16_t intervalms = interfaces.get_inactive_module_count() > 0 ? 300 : 1000;
  if (ModuleVariableSet::out_of_memory) intervalms = 30;
  if (mi_interval_elapsed(last_led_change, intervalms)) {
    static bool led_on = false;
    digitalWrite(LED_BUILTIN, led_on ? HIGH : LOW);
    led_on = !led_on;
  }
}