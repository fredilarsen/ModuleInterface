/* This is the master for the full ModuleInterface example setup which includes:
* 1. A master (Arduino Mega plus Ethernet shield)
* 2. A module reading the ambient light level (Nano, Uno, ...)
* 3. A module which turns the light on and off (on-board LED for now) (Nano, Uno, ...)
* 4. A webserver+database setup which keeps all the module configuration and logs their output
* 5. A web site that shows the status of the modules, allows their configuration to be changed,
*    and to view their output as instant values or in trend plots.
*/

// Define one of these modes
#define ETCP_SINGLE_DIRECTION
//#define ETCP_SINGLE_SOCKET_WITH_ACK

#define PJON_INCLUDE_ETCP

#ifdef _WIN32
  // MS compiler does not like PJON_MAX_PACKETS=0 in PJON
  #define PJON_MAX_PACKETS 1
#endif

#include <MIMaster.h>

// PJON related
PJONLink<EthernetTCP> bus(45); // PJON device id 1

// Web server related
const uint8_t web_server_ip[4] = { 192, 1, 1, 169 };

EthernetClient web_client;

// Module interfaces
PJONModuleInterfaceSet interfaces(bus, "SensMon:sm:10 LightCon:lc:20", "m1");
MIHttpTransfer http_transfer(interfaces, web_client, web_server_ip);

void setup() {
  printf("Welcome to ModuleMasterHttp (ETCP).\n");
  bus.bus.strategy.link.set_id(bus.get_id());
  bus.bus.strategy.link.keep_connection(true);
#ifdef ETCP_SINGLE_DIRECTION
  bus.bus.strategy.link.single_initiate_direction(true);
#endif
#ifdef ETCP_SINGLE_SOCKET_WITH_ACK
  bus.bus.strategy.link.single_socket(true);
#endif
  bus.bus.strategy.link.start_listening();
  bus.bus.begin();

  // Set frequency of transfer between modules
  interfaces.sampling_time_settings = 1000;
  interfaces.sampling_time_outputs = 1000;
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
