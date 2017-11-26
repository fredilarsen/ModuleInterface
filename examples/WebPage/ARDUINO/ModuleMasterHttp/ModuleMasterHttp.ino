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
byte gateway[] = { 192, 1, 1, 1 };
byte subnet[] = { 255, 255, 255, 0 };
byte mac[] = {0xDE, 0xCD, 0x4E, 0xEE, 0xEE, 0xED};
byte ip[] = { 192, 1, 1, 181 };

// Module interfaces
PJONModuleInterfaceSet interfaces(bus, "SensMon:sm:10 LightCon:lc:20", "m1");


void setup() {
  Ethernet.begin(mac, ip, gateway, gateway, subnet);
  
  bus.bus.strategy.set_pin(7);
  bus.bus.begin();
  interfaces.sampling_time_outputs = 1000;
  
  // Init on-board LED
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);  
}


void loop() {
  // Do data exchange to and from and between the modules
  interfaces.update();
    
  // Get settings for each module from the database via the web server
  static uint32_t last_s = 0;
  if (mi_interval_elapsed(last_s, 10000))
    get_settings_from_web_server(interfaces, web_client, web_server_ip);

  // Store all measurements to the database via the web server
  static uint32_t last_v = millis();
  if (mi_interval_elapsed(last_v, 10000)) {
    static MILastScanTimes last_scan_times;
    // (set primary_master=false on all masters but one if there are more than one)
    send_values_to_web_server(interfaces, web_client, web_server_ip, &last_scan_times); 
  }
  
  // Let activity flash go to rapid if one or more modules are inactive, faster if low mem
  static uint32_t last_led_change = millis();
  uint16_t intervalms = interfaces.get_inactive_module_count() > 0 ? 300 : 1000;
  if (ModuleVariableSet::out_of_memory) intervalms = 30;
  if (mi_interval_elapsed(last_led_change, intervalms)) {
    static bool led_on = false;
    digitalWrite(13, led_on ? HIGH : LOW);
    led_on = !led_on;
  }
}
