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
#define MI_USE_SYSTEMTIME

#ifdef _WIN32
  // MS compiler does not like PJON_MAX_PACKETS=0 in PJON
  #define PJON_MAX_PACKETS 1
#endif

#include <MIMaster.h>

// PJON related
PJONLink<LocalUDP> bus; // PJON device id 1

// Web related
EthernetClient web_client;

// Module interfaces
PJONModuleInterfaceSet interfaces(bus, (const char *)NULL);
PJONMIHttpTransfer http_transfer(interfaces, web_client, NULL, 1000, 10000);

void parse_ip_string(const char *ip_string, in_addr &ip) {
  if (!inet_pton(AF_INET, ip_string, &ip)) {
    printf("ERROR: IP address '%s' in not in a valid notation.\n", ip_string);
    exit(1);
  }  
}

void setup(int argc, const char * const argv[]) {
  printf("Welcome to GenericModuleMasterHttp.\n");

  if (argc < 2) {
    printf("ERROR: The IP address of a web server must be specified on the command line.\n");
    printf("Parameters: <IPv4 address in dot notation> [<port number> [<master prefix>]]\n");
    printf("(Default port number is 80. Default master prefix is 'm1'.)\n");
    exit(2);
  }

  // Parse web server IP address specified on command line
  String webserver = argv[1];
  in_addr web_server_ip;
  parse_ip_string(webserver.c_str(), web_server_ip);
  printf("Using web server at %s.\n", webserver.c_str());

  // Get the web server port number if present
 uint16_t port_number = 80;
  if (argc > 2) {
    port_number = atoi(argv[2]);
    if (port_number == 0) {
      printf("ERROR: Web server port number '%s' must be a valid integer.\n", argv[2]);
      exit(3);
    }
  }

  // Get the master prefix
  String master_prefix = "m1";
  if (argc > 3) {
    master_prefix = argv[3];
    if (master_prefix.length() != 2) {
      printf("ERROR: Master prefix '%s' must have length 2.\n", master_prefix.c_str());
      exit(4);
    }
  }

  // Now get the rest of the configuration from the web server
  bus.bus.strategy.set_port(7200); // Use the same port on all devices
  bus.bus.begin();
  interfaces.set_prefix(master_prefix.c_str());
  http_transfer.set_primary_master(strcmp(master_prefix.c_str(), "m1")==0); // Responsible for archiving
  http_transfer.set_web_server_address((uint8_t*) &web_server_ip);
  http_transfer.set_web_server_port(port_number);
  if (!http_transfer.get_master_settings_from_server()) {
    printf("ERROR: Could not retrieve master settings from web server. Are settings present in database?\n");
    exit(4);
  }
}

void loop() {
  interfaces.update();    // Data exchange to and from and between the modules
  http_transfer.update(); // Data exchange to and from web server
  delay(10);
}

int main(int argc, const char * const argv[]) {
  setup(argc, argv);
  while (true) loop();
  return 0;
}

