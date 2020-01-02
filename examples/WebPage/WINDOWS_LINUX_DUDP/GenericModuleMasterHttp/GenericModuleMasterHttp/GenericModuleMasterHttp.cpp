/* This is the master for the full ModuleInterface example setup which includes:
* 1. A master (this program, on Linux or Windows)
* 2. A DUDP<->SWBB Switch (Nano/Uno/Mega with Ethernet shield)
* 3. A module reading the ambient light level (Nano, Uno, ...)
* 4. A module which turns the light on and off (on-board LED for now) (Nano, Uno, ...)
* 5. A webserver+database setup which keeps all the module configuration and logs their output
* 6. A web site that shows the status of the modules, allows their configuration to be changed,
*    and to view their output as instant values or in trend plots.
* 7. (Optional) Transfer of settings, outputs and inputs through a MQTT broker for easy
*    connection to/from systems like Home Assistant and OpenHAB.
*/

#define PJON_INCLUDE_DUDP
#define MI_USE_SYSTEMTIME
#define DEBUG_PRINT_TIMES
#define MI_USE_MQTT
#define MASTER_MULTI_TRANSFER
//#define DEBUG_PRINT_SETTINGSYNC
//#define MQTT_DEBUGPRINT
//#define MIMQTT_USE_JSON

// We have memory enough, so allow the maximum amount of nodes
#define DUDP_MAX_REMOTE_NODES 255

#define DUDP_RESPONSE_TIMEOUT 100000ul

#ifdef _WIN32
  // MS compiler does not like PJON_MAX_PACKETS=0 in PJON
  #define PJON_MAX_PACKETS 1
#endif

#include <MIMaster.h>

// PJON related
PJONLink<DualUDP> bus; // PJON device id 1

// Web related
EthernetClient web_client;

// Module interfaces
PJONModuleInterfaceSet interfaces(bus, (const char *)NULL);
PJONMIHttpTransfer http_transfer(interfaces, web_client, NULL);

#ifdef MI_USE_MQTT
#include <MI/ModuleInterfaceMqttTransfer.h>
MIMqttTransfer mqtt_transfer(interfaces, NULL);
MITransferBase *transfers[2] = { &http_transfer, &mqtt_transfer };
#else
MITransferBase *transfers[1] = { &http_transfer };
#endif

void parse_ip_string(const char *ip_string, in_addr &ip) {
  if (!inet_pton(AF_INET, ip_string, &ip)) {
    printf("ERROR: IP address '%s' in not in a valid notation.\n", ip_string);
    exit(1);
  }  
}

//void notification_function(NotificationType notification_type, const ModuleInterface *interface);

void setup(int argc, const char * const argv[]) {
  printf("Welcome to GenericModuleMaster (DualUDP+HTTP+MQTT).\n");

  if (argc < 2) {
    printf("ERROR: The IP address of a web server must be specified on the command line.\n");
    printf("Parameters: <IPv4 address in dot notation> [<port number> [<master prefix> [<MQTT port]]]\n");
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

  // Get MQTT address
  String mqtt_address;
  if (argc > 4) {
#ifdef MI_USE_MQTT
    mqtt_address = argv[4];
#else
    printf("ERROR: MQTT support not compiled into this executable!\n");
    exit(5);
#endif
  }

  // Now get the rest of the configuration from the web server
  bus.bus.begin();
  interfaces.set_prefix(master_prefix.c_str());
  http_transfer.set_primary_master(strcmp(master_prefix.c_str(), "m1")==0); // Responsible for archiving
  http_transfer.set_web_server_address((uint8_t*) &web_server_ip);
  http_transfer.set_web_server_port(port_number);
  if (!http_transfer.get_master_settings_from_server()) {
    printf("ERROR: Could not retrieve master settings from web server. Are settings present in database?\n");
    exit(6);
  }

#ifdef MI_USE_MQTT
  // Set up MQTT connection
  if (!mqtt_address.empty()) {
//    interfaces.set_notification_callback(notification_function);
    //mqtt_transfer.set_address("tcp://192.1.1.71:1883"); // TODO: Support URL syntax
    mqtt_transfer.set_broker_address((uint8_t*)&web_server_ip, 1883);
    mqtt_transfer.start();
  }
#endif
}

void loop() {
  // Check for new master settings regularly, to support dynamic addition of modules
  uint32_t last_mastersettings_check = millis();
  if (mi_interval_elapsed(last_mastersettings_check, 60000))
    http_transfer.get_master_settings_from_server();

  interfaces.update(sizeof transfers / sizeof transfers[0], transfers);
  delay(1);
}

int main(int argc, const char * const argv[]) {
  setup(argc, argv);
  while (true) loop();
  return 0;
}

/*
void notification_function(NotificationType notification_type, const ModuleInterface *interface) {
  if (!interface) return;
  if (notification_type == ntNewOutputs) { // New output from a module
#ifdef MI_USE_MQTT
    mqtt_transfer.put_values(*interface); // Publish module outputs to MQTT topic
#endif
  }
  else if (notification_type == ntNewSettings) { // New settings from a module
#ifdef MI_USE_MQTT
    mqtt_transfer.put_settings(*interface); // Publish module settings to MQTT topic
#endif
  }
}
*/


