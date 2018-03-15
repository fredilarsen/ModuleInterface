/* This is the master for the full ModuleInterface example setup which includes:
* 1. A master (this program, on Linux or Windows)
* 2. A LUDP<->SWBB Switch (Nano or Uno with Ethernet shield)
* 3. A module reading the ambient light level (Nano, Uno, ...)
* 4. A module which turns the light on and off (on-board LED for now) (Nano, Uno, ...)
* 5. A webserver+database setup which keeps all the module configuration and logs their output
* 6. A web site that shows the status of the modules, allows their configuration to be changed,
*    and to view their output as instant values or in trend plots.
*/

#define PJON_INCLUDE_LUDP

#include <MIMaster.h>

// PJON related
PJONLink<LocalUDP> bus(1); // PJON device id 1

// Web server related
const uint8_t web_server_ip[4] = { 192, 1, 1, 169 };

EthernetClient web_client;

// Module interfaces
PJONModuleInterfaceSet interfaces(bus, "SensMon:sm:10 LightCon:lc:20", "m1");
MIHttpTransfer http_transfer(interfaces, web_client, web_server_ip, 1000, 10000);

void setup() {
  printf("Welcome to ModuleMasterHttp.\n");
  bus.bus.strategy.link.set_port(7200); // Use the same port on all devices
  bus.bus.begin();

  // Set frequency of transfer between modules
  interfaces.sampling_time_settings = 1000;
  interfaces.sampling_time_outputs = 1000;
}

void loop() {
  interfaces.update();    // Data exchange to and from and between the modules
  http_transfer.update(); // Data exchange to and from web server
  delay(10);
}

int main() {
  setup();
  while (true) loop();
  return 0;
}
