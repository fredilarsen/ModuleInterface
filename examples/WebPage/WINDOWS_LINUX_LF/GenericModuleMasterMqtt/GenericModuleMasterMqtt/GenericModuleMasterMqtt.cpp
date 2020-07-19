/* This example demonstrates how to connect your ModuleInterface based setup to
*  a MQTT broker. Settings, outputs and inputs are transferred through a MQTT broker 
*  for easy connection to/from systems like Home Assistant and OpenHAB.
*
* This example uses _only_ a connection to a MQTT broker, and no connection to the ModuleInterface
* web pages or databases. See the other master example that is connected to both systens at once.
* This example also uses the PJON LocalFile strategy to make it easy to run master and modules as 
* processes on the same computer, for test purposes. No extra hardware needed.
* Notice how the list of modules is hardcoded in this example while it is retrieved dynamically 
* from the database in the generic master example.
*/

#define MI_USE_SYSTEMTIME
#define DEBUG_PRINT_TIMES
#define MI_USE_MQTT
#define MASTER_MULTI_TRANSFER
//#define DEBUG_PRINT_SETTINGSYNC
//#define MQTT_DEBUGPRINT
//#define MIMQTT_USE_JSON

#ifdef _WIN32
  // MS compiler does not like PJON_MAX_PACKETS=0 in PJON
  #define PJON_MAX_PACKETS 1
#endif

#include <MIMaster.h>
#include <PJONLocalFile.h>
#include <MI/ModuleInterfaceMqttTransfer.h>

// PJON related
PJONLink<LocalFile> bus(1); // PJON device id 1

// Module interfaces
PJONModuleInterfaceSet interfaces(bus, (const char *)NULL);

MIMqttTransfer mqtt_transfer(interfaces, NULL);
MITransferBase *transfers[1] = { &mqtt_transfer };

void parse_ip_string(const char *ip_string, in_addr &ip) {
  if (!inet_pton(AF_INET, ip_string, &ip)) {
    printf("ERROR: IP address '%s' in not in a valid notation.\n", ip_string);
    exit(1);
  }  
}

void setup(int argc, const char * const argv[]) {
  printf("Welcome to GenericModuleMaster (LocalFile+MQTT).\n");

  if (argc < 2) {
    printf("ERROR: The IP address of a MQTT broker must be specified on the command line.\n");
    printf("Parameters: <IPv4 address in dot notation> [<MQTT port number> [<master prefix>]]\n");
    printf("(Default port number is 1884. Default master prefix is 'm1'.)\n");
    exit(2);
  }

  // Parse MQTT broker IP address specified on command line
  String ip = argv[1];
  in_addr broker_ip;
  parse_ip_string(ip.c_str(), broker_ip);
  printf("Using MQTT broker at %s.\n", ip.c_str());

  // Get the MQTT port number if present
  uint16_t port_number = 1883;
  if (argc > 2) {
    port_number = atoi(argv[2]);
    if (port_number == 0) {
      printf("ERROR: MQTT broker port number '%s' must be a valid integer.\n", argv[2]);
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

  bus.bus.begin();
  interfaces.set_prefix(master_prefix.c_str());
  interfaces.set_transfer_interval(20000);
  interfaces.set_interface_list("EvtTest:et:20");  

  // Set up MQTT connection
  mqtt_transfer.set_broker_address((uint8_t*)&broker_ip, port_number);
  mqtt_transfer.start();

  interfaces.set_external_transfer(sizeof transfers / sizeof transfers[0], transfers);
}

void loop() {
  interfaces.update();
  delay(1);
}

int main(int argc, const char * const argv[]) {
  setup(argc, argv);
  while (true) loop();
  return 0;
}



