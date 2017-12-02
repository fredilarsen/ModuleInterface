/* This is the master for the full ModuleInterface example setup which includes:
 * 1. A master (Arduino Mega plus Ethernet shield)
 * 2. A module reading the ambient light level (Nano, Uno, ...)
 * 3. A module which turns the light on and off (on-board LED for now) (Nano, Uno, ...)
 * 4. A webserver+database setup which keeps all the module configuration and logs their output
 * 5. A web site that shows the status of the modules, allows their configuration to be changed,
 *    and to view their output as instant values or in trend plots.
 */

#include <MiMaster.h>

// Ethernet configuration for this device
IPAddress gateway = { 192, 1, 1, 1 };
IPAddress subnet = { 255, 255, 255, 0 };
IPAddress local_ip = { 192, 1, 1, 186 };

// Address of modules
uint8_t remote_ip10[] = { 192, 1, 1, 187 };
uint8_t remote_ip20[] = { 192, 1, 1, 188 };

// Web server related
IPAddress web_server_ip = { 192, 1, 1, 143};
WiFiClient web_client;

// WiFi settings
const char* ssid     = "MyNetworkSSID";
const char* password = "MyPassword";

PJONLink<GlobalUDP> link(1); // PJON device id 1

// Module interfaces
PJONModuleInterfaceSet interfaces(link, "SensMon:sm:10 LightCon:lc:20", "m1");

void setup() {
  Serial.begin(115200);
  Serial.println("ModuleMasterHttp started.");
  
  WiFi.mode(WIFI_STA); // Be a client and not an access point
  WiFi.config(local_ip, gateway, subnet);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("Now listening at IP %s\n", WiFi.localIP().toString().c_str());

  link.bus.strategy.add_node(10, remote_ip10);
  link.bus.strategy.add_node(20, remote_ip20);
  link.bus.begin();
  interfaces.sampling_time_outputs = 1000;
  interfaces.sampling_time_settings = 1000;
    
  // Init on-board LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);  
}

void loop() {
  // Do data exchange to and from and between the modules
  interfaces.update();

  // Get settings for each module from the database via the web server
  static uint32_t last_s = 0;
  if (mi_interval_elapsed(last_s, 1000))
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
    digitalWrite(LED_BUILTIN, led_on ? LOW : HIGH);
    led_on = !led_on;
  }  
}
