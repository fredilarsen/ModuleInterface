/* This is the master for the full ModuleInterface example setup which includes:
 * 1. A master (Arduino Mega plus Ethernet shield)
 * 2. A module reading the ambient light level (Nano, Uno, ...)
 * 3. A module which turns the light on and off (on-board LED for now) (Nano, Uno, ...)
 * 4. A webserver+database setup which keeps all the module configuration and logs their output
 * 5. A web site that shows the status of the modules, allows their configuration to be changed,
 *    and to view their output as instant values or in trend plots.
 */

#define MI_HTTPCLIENT
#include <MIMaster.h>

// Web server related
uint8_t web_server_ip[] = { 192, 1, 1, 143};
WiFiClient web_client;

// WiFi settings
const char* ssid     = "MyNetworkSSID";
const char* password = "MyPassword";

PJONLink<DualUDP> pjonlink(1); // PJON device id 1

// Module interfaces
PJONModuleInterfaceSet interfaces(pjonlink, "SensMon:sm:10 LightCon:lc:20", "m1");
MIHttpTransfer http_transfer(interfaces, web_client, web_server_ip);

void setup() {
  Serial.begin(115200);
  Serial.println("ModuleMasterHttp started.");
  
  WiFi.mode(WIFI_STA); // Be a client and not an access point
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("Now listening at IP %s\n", WiFi.localIP().toString().c_str());
  pjonlink.bus.begin();
  interfaces.sampling_time_outputs = 1000;
  interfaces.sampling_time_settings = 1000;
    
  // Init on-board LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);  
}

void loop() {
  interfaces.update(&http_transfer); // Do all data exchange
  flash_status_led();     // Show module status by flashing LED
}

void flash_status_led() {  
  // Let activity flash go to rapid if one or more modules are inactive, faster if low mem
  static uint32_t last_led_change = millis();
  uint16_t intervalms = interfaces.get_inactive_module_count() > 0 ? 300 : 1000;
  if (mvs_out_of_memory) intervalms = 30;
  if (mi_interval_elapsed(last_led_change, intervalms)) {
    static bool led_on = false;
    digitalWrite(LED_BUILTIN, led_on ? LOW : HIGH);
    led_on = !led_on;
  }  
}