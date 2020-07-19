/* This is the master for the full ModuleInterface example setup which includes:
* 1. A master (Linux or Windows)
* 2. A LUDP<->SWBB switch (Nano or Uno with Ethernet shield)
* 3. A module reading the ambient light level (Nano, Uno, ...)
* 4. A module which turns the light on and off (on-board LED for now) (Nano, Uno, ...)
* 5. A webserver+database setup which keeps all the module configuration and logs their output
* 6. A web site that shows the status of the modules, allows their configuration to be changed,
*    and to view their output as instant values or in trend plots.
*/

#define DEBUG_PRINT
#define MI_USE_SYSTEMTIME

#ifdef _WIN32
  // MS compiler does not like PJON_MAX_PACKETS=0 in PJON
  #define PJON_MAX_PACKETS 1
#endif

#include <MIMaster.h>
#include <PJONDualUDP.h>

// PJON related
PJONLink<DualUDP> bus(1); // PJON device id 1

// Web server related
const uint8_t web_server_ip[4] = { 192, 1, 1, 169 };

EthernetClient web_client;

// Module interfaces
PJONModuleInterfaceSet interfaces(bus, "SensMon:sm:10 LightCon:lc:20", "m1");
MIHttpTransfer http_transfer(interfaces, web_client, web_server_ip);

void setup() {
  printf("Welcome to ModuleMasterHttp (DualUDP).\n");
  bus.bus.begin();

  // Set frequency of transfer between modules
  interfaces.set_transfer_interval(2000);
}

void loop() {
  interfaces.update(&http_transfer); // Do all data exchange
  delay(10);
}

int main() {
  setup();
  while (true) loop();
  return 0;
}
