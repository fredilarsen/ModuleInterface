/* This ModuleInterface example sketch simply reads and reports some sensor values */

#include <MIModule.h>
#include <PJONDualUDP.h>

// WiFi settings
const char* ssid     = "MyNetworkSSID";
const char* password = "MyPassword";

PJONLink<DualUDP> pjonlink(10); // PJON device id 10

PJONModuleInterface interface("Outdoor",                          // Module name
                              pjonlink,                           // PJON bus
                              "",                                 // Settings
                              "",                                 // Inputs
                              "Light:u2 LightLP:f4 Motion:u1");   // Outputs (measurements)

// Outputs (measurements) (index/position in outputs list)
#define o_light_ix        0
#define o_lowpasslight_ix 1
#define o_motion_ix       2

#define PIN_LIGHTSENSOR  A0 // Analog light sensor connected to this pin
#define PIN_MOTIONSENSOR 0  // Motion sensor connected to this pin

void setup() { 
  Serial.begin(115200);
  Serial.println("SensorMonitor started.");

  WiFi.mode(WIFI_STA); // Be a client and not an access point
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("Now listening at IP %s\n", WiFi.localIP().toString().c_str());
  pjonlink.bus.begin();  
  
  pinMode(PIN_LIGHTSENSOR, INPUT);
  pinMode(PIN_MOTIONSENSOR, INPUT);   
}

void loop() { interface.update(); read_sensors(); }

void read_sensors() {
  static uint32_t last_time = millis();
  if (mi_interval_elapsed(last_time, 100)) {
    // Read light sensor, and maintain a low pass filtered value
    uint16_t light = 1023 - analogRead(PIN_LIGHTSENSOR);
    static float lowpasslight = light;
    lowpasslight = mi_lowpass(light, lowpasslight, 0.001f);
  
    // Register the outputs
    interface.outputs.set_value(o_light_ix, light);
    interface.outputs.set_value(o_lowpasslight_ix, lowpasslight);
    interface.outputs.set_value(o_motion_ix, digitalRead(PIN_MOTIONSENSOR));
    interface.outputs.set_updated(); // Flag as completely updated, all values are set
  }
}