/* Similar to the Switch but with a blinking RGB led indicating each packet 
   that is forwarded with blue and green for each direction respectively, 
   plus errors in red.
   RED (pin 4)   - Packet delivery attempted but failed
   GREEN (pin 5) - Forwarded packet from SWBB bus to LUDP bus
   BLUE (pin 6)  - Forwarded packet from LUDP bus to SWBB bus

   Every 10s the LED will flash green as a "heartbeat",
   if no other traffic has passed.
   
   This ModuleInterface version of the PJON BlinkingRGBSwitch example
   will report its uptime and traffic statistics to the MI master
   for analysis and plotting.
   
   This sketch will just fit into an Arduino Nano, but if you need to
   increase the maximum packet size or add more code or add a queue for
   increased performance, you will need to switch to a Mega or
   some other device.
*/

#include "ModuleInterfaceRGBSwitch.h"

// Ethernet configuration for this device, MAC must be unique!
byte mac[] = {0xDE, 0x34, 0x34, 0xEF, 0xFE, 0xE1};

// PJON device id on first bus (DualUDP)
const uint8_t PJON_ID = 40;

StrategyLink<DualUDP> link1;
StrategyLink<SoftwareBitBang> link2;

PJONAny bus1(&link1, PJON_ID);
PJONAny bus2(&link2, PJON_NOT_ASSIGNED, 10000);

ModuleInterfaceRGBSwitch router(2, (PJONAny*[2]){&bus1, &bus2});

void notification_function(NotificationType notification_type, const ModuleInterface *p) {
  router.notification_function(notification_type, p);
}

void setup() {
  #ifdef DEBUG_PRINT
  Serial.begin(115200);
  Serial.println(F("Welcome to ModuleInterfaceRGBSwitch (LUDP-SWBB)."));
  #endif
  while (Ethernet.begin(mac) == 0) ; // Wait for DHCP response
  link2.strategy.set_pin(7);
  router.set_virtual_bus(0); // Enable virtual bus
  router.set_notification_callback(notification_function);
  router.begin();
}

void loop() {
  router.update();
  byte status = Ethernet.maintain(); // Maintain DHCP lease
  if (status == 1 || status == 3) {
    // Need to refresh lease
    while (Ethernet.begin(mac) == 0); // Wait for DHCP response
  }

  // Blink red light if we had to broadcast. This should happen only during startup,
  // and if it appears otherwise it could flag a need for increasing the node table size.
  if (link1.strategy.did_broadcast()) digitalWrite(ERROR_LED_PIN, HIGH);
}
