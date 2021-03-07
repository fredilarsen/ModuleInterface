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

#define MI_USE_SYSTEMTIME
#define MASTER_MULTI_TRANSFER
//#define MIMQTT_USE_JSON

#define DEBUG_PRINT_TIMES
//#define DEBUG_PRINT_SETTINGUPDATE_MQTT
//#define DEBUG_PRINT_SETTINGSYNC
//#define MQTT_DEBUGPRINT

// We have memory enough, so allow the maximum amount of nodes
#define DUDP_MAX_REMOTE_NODES 255

#define DUDP_RESPONSE_TIMEOUT 100000ul

#ifdef _WIN32
  // MS compiler does not like PJON_MAX_PACKETS=0 in PJON
  #define PJON_MAX_PACKETS 1
#endif

#include <MIMaster.h>
#include <PJONDualUDP.h>
#include <MI_PJON/PJONModuleInterfaceMqttTransfer.h>

// ModuleInterface objects
PJONLink<DualUDP> bus;
PJONModuleInterfaceSet interfaces(bus, (const char *)NULL);

// HTTP related
EthernetClient web_client;
PJONMIHttpTransfer http_transfer(interfaces, web_client, NULL);

// MQTT related
PJONMIMqttTransfer mqtt_transfer(interfaces, false, NULL);

MITransferBase *transfers[2] = { &http_transfer, &mqtt_transfer };
bool http_config_source = false, mqtt_config_source = false;


bool is_switch(const char *word) { return word != NULL && word[0] == '-'; }

void print_instructions() {
  printf("A command line must be given. The syntax is:\n");
  printf("  GenericModuleMaster[.exe] <parameters>\n");
  printf("Where <parameters> consists of one or more of these:\n");
  printf("  [-http <server_ip> [<server_port>]]\n");
  printf("  [-mqtt <server_ip> [<port>]]\n");
  printf("  [-config http|mqtt]\n");
  printf("  [-prefix <prefix>]\n");
  printf("IP adresseses are in IPv4 dot notation.\n");
  printf("Default http port number is 80. Default mqtt port number is 1883.\n");
  printf("Default master prefix is 'm1'.\n");
}

void parse_ip_string(const char *ip_string, in_addr &ip) {
  if (!inet_pton(AF_INET, ip_string, &ip)) {
    printf("ERROR: IP address '%s' in not in a valid notation.\n", ip_string);
    exit(2);
  }  
}

int get_ip_and_port(int argc, const char * const argv[], int ix, in_addr &ip, uint16_t &port) {
  int start_ix = ix;
  if (argc > ix && !is_switch(argv[ix])) {
    parse_ip_string(argv[ix], ip);
    ix++;
  } else {
    printf("ERROR: Missing IP address in command line.\n\n");
    print_instructions();
    exit(3);
  }
  if (argc > ix && !is_switch(argv[ix])) {
    uint16_t p = (uint16_t) atoi(argv[ix]);
    if (p > 0 && p < (1 << 16) - 1) port = p; // Valid port specified
    else {
      printf("ERROR: Expected port number instead of '%s'.\n", argv[ix]);
      exit(4);
    }
    ix++;
  }
  return ix - start_ix; // Number of arguments read
}

void setup(int argc, const char * const argv[]) {
  if (argc < 2) {
    print_instructions();
    exit(1);
  }

  // Parse command line
  uint16_t http_server_port = 80, mqtt_server_port = 1883;
  in_addr http_server_ip, mqtt_server_ip;
  memset(&http_server_ip, 0, sizeof http_server_ip); 
  memset(&mqtt_server_ip, 0, sizeof mqtt_server_ip); 
  const char *config_source = "http", *master_prefix = "m1";
  for (uint8_t i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-http")==0) i += get_ip_and_port(argc, argv, i + 1, http_server_ip, http_server_port);
    else if (strcmp(argv[i], "-mqtt")==0) i += get_ip_and_port(argc, argv, i + 1, mqtt_server_ip, mqtt_server_port);
    else if (strcmp(argv[i], "-config")==0) { // Source for master config if both http and mqtt
      if (argc > i+1) config_source = argv[i + 1];
      i++;
    }
    else if (strcmp(argv[i], "-prefix")==0) { // Master prefix if multiple masters
      if (argc > i+1) {
        master_prefix = argv[i + 1];
        if (strlen(master_prefix) != 2) {
          printf("ERROR: Master prefix '%s' must have length 2.\n", master_prefix);
          exit(5);
        }
        i++;
      }
    }
  }
  bool use_http = *(uint32_t*)&http_server_ip != 0,
       use_mqtt = *(uint32_t*)&mqtt_server_ip != 0;
  if (!use_http && !use_mqtt) {
      printf("ERROR: Either HTTP or MQTT or both must be enabled on the command line.\n\n");
      print_instructions();
      exit(6);
  }
  printf("Using master prefix '%s'.\n", master_prefix);
  if (use_http) {
    const char *ipstring = inet_ntoa(http_server_ip);
    printf("Using HTTP server at %s port %d.\n", ipstring == NULL ? "?" : ipstring, http_server_port);
  }
  if (use_mqtt) {
    const char *ipstring = inet_ntoa(http_server_ip);
    printf("Using MQTT server at %s port %d.\n", ipstring == NULL ? "?" : ipstring, mqtt_server_port);
  }

  // Determine where to read the master's configuration from
  mqtt_config_source = use_mqtt && (!use_http || strcmp("mqtt", config_source) == 0);
  http_config_source = !mqtt_config_source;
  printf("Master settings from the %s server will be used.\n", mqtt_config_source ? "MQTT" : "HTTP");

  // Now get the rest of the configuration from the web server
  bus.bus.begin();
  interfaces.set_prefix(master_prefix);
  if (use_http) {
    http_transfer.set_primary_master(strcmp(master_prefix, "m1")==0); // Responsible for archiving
    http_transfer.set_web_server_address((uint8_t*) &http_server_ip);
    http_transfer.set_web_server_port(http_server_port);
    if (http_config_source && !http_transfer.get_master_settings_from_server()) {
      printf("ERROR: Could not retrieve master settings from web server. Are settings present in database?\n");
      exit(7);
    }
  }

  // Set up MQTT connection
  if (use_mqtt) {
    //mqtt_transfer.set_address("tcp://192.1.1.71:1883"); // TODO: Support URL syntax
    mqtt_transfer.set_broker_address((uint8_t*)&mqtt_server_ip, mqtt_server_port);
    mqtt_transfer.set_read_master_settings(mqtt_config_source);
    mqtt_transfer.start();
  }

  // Register HTTP and/or MQTT transports
  uint8_t len = use_http && use_mqtt ? 2 : 1,
          ix = len == 1 && use_mqtt ? 1 : 0;
  interfaces.set_external_transfer(len, &transfers[ix]);
}

void loop() {
  if (http_config_source) {
    // Check for new master settings regularly, to support dynamic addition of modules.
    uint32_t last_mastersettings_check = millis();
    if (mi_interval_elapsed(last_mastersettings_check, 60000))
      http_transfer.get_master_settings_from_server();
  }

  interfaces.update();
  delay(1);
}

int main(int argc, const char * const argv[]) {
  printf("Welcome to GenericModuleMaster v2.0 (DualUDP+HTTP+MQTT).\n");
  setup(argc, argv);
  printf("Initialization completed, starting data exchange.\n");
  while (true) loop();
  return 0;
}



