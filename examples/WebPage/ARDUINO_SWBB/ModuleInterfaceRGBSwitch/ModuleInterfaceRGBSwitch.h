#pragma once

/* Similar to the Switch but with a blinking RGB led indicating each packet 
   that is forwarded with blue and green for each direction respectively, 
   plus errors in red.
   RED (pin 4)   - Packet delivery attempted but failed
   GREEN (pin 5) - Forwarded packet from SWBB bus to LUDP bus
   BLUE (pin 6)  - Forwarded packet from LUDP bus to SWBB bus

   Every 10s the LED will flash green as a "heartbeat",
   if no other traffic has passed.
   
   This ModuleInterface version of the BlinkingRGBSwitch will
   report its uptime and traffic statistics to the MI master
   for analysis and plotting.
*/

//#define DEBUG_PRINT
#define PJON_MAX_PACKETS 0
#define PJON_PACKET_MAX_LENGTH 180
#include <PJONInteractiveRouter.h>
#include <MIModule.h>
#include <MI_PJON/PJONPointerLink.h>
#include <utils/MIUptime.h>
#include <utility/w5100.h>

// Pins for blinking RGB LED for showing traffic in each direction
// (Use resistors approximately R:3.3k G:33k, B:8.2k)
// A clear, not diffuse, RGB LED lets the individual colors be seen directly.
const int ERROR_LED_PIN = 4, SWBB_LED_PIN = 5, LUDP_LED_PIN = 6;

const char PROGMEM output_names[]  = "Uptime:u4 Elapsed:u4 CntToSW:uf4 CntFrSW:f4 BytToSW:f4 BytFrSW:f4 SockFree:u1",
                   setting_names[] = "", input_names[] = ""; 

class ModuleInterfaceRGBSwitch : public PJONModuleInterface, public PJONVirtualBusRouter<PJONSwitch> {

  void dynamic_receiver_function(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info) {
    // Is this a MI packet for me?  
// TODO: Support packets with bus id
    if (current_bus == 0 && (packet_info.receiver_id == link.get_id() || packet_info.receiver_id == 0)) {
      if (packet_info.receiver_id != 0) link.get_bus()->strategy.send_response(PJON_ACK);
      PJONModuleInterface::handle_message(payload, length, packet_info);
      if (packet_info.receiver_id != 0) return;
    }
    PJONVirtualBusRouter<PJONSwitch>::dynamic_receiver_function(payload, length, packet_info);
  }
  void send_packet(const uint8_t *payload, const uint16_t length,
                   const uint8_t receiver_bus, const uint8_t sender_bus,
                   bool &ack_sent, const PJON_Packet_Info &packet_info) {
                    
    uint8_t p = sender_bus == 1 ? SWBB_LED_PIN : LUDP_LED_PIN;
    digitalWrite(p, HIGH);
    PJONVirtualBusRouter<PJONSwitch>::send_packet(payload, length, receiver_bus, sender_bus, ack_sent, packet_info);
    digitalWrite(p, LOW);
    packet_count[current_bus]++;
    byte_count[current_bus] += length;
    digitalWrite(ERROR_LED_PIN, LOW);
    last_led_activity = millis();
  }
  void dynamic_error_function(uint8_t code, uint16_t data) {
    uint8_t p = ERROR_LED_PIN; // RED
    digitalWrite(p, HIGH);
    PJONVirtualBusRouter<PJONSwitch>::dynamic_error_function(code, data);
    last_led_activity = millis();
    //digitalWrite(p,LOW);
  }

    void restart_statistics() { 
    packet_count[0] = packet_count[1] = byte_count[0] = byte_count[1] = 0;
    statistics_start = millis();
  }

  // Support for a heartbeat flash if there is no other activity, to show life signs
  uint32_t last_led_activity = millis(),
           statistics_start;

  // Statistics
  uint16_t packet_count[2], byte_count[2];
  uint8_t free_sockets = 0;
  uint32_t sock_scan_time = 0;

  PJONPointerLink<Any> link;

  uint8_t get_free_socket_count() {
    // 0=avail,14=waiting,17=connected,22=UDP,1C=close wait
    uint8_t cnt = 0;
    for (uint8_t i = 0; i < MAX_SOCK_NUM; i++) {
      uint8_t s = W5100.readSnSR(i);
      if (s == 0) cnt++;
    }
    return cnt;
  }
 
public:
  ModuleInterfaceRGBSwitch(uint8_t bus_count, PJONAny *buses[], uint8_t default_gateway = PJON_NOT_ASSIGNED) 
    : PJONModuleInterface("", link, true, setting_names, input_names, output_names), 
      PJONVirtualBusRouter<PJONSwitch>(bus_count, buses, default_gateway)
    {
    // Let PJONModuleInterface use the same bus as the router
    link.set_bus(&PJONVirtualBusRouter<PJONSwitch>::get_bus(0));
    restart_statistics();

    // Init pins for external LEDs
    pinMode(ERROR_LED_PIN, OUTPUT);
    pinMode(SWBB_LED_PIN, OUTPUT);
    pinMode(LUDP_LED_PIN, OUTPUT);
  }
  
  void update() {
    if (mi_interval_elapsed(sock_scan_time, 10000)) free_sockets = get_free_socket_count();

    PJONVirtualBusRouter<PJONSwitch>::loop();
    PJONModuleInterface::send_output_events();
    // Turn off LEDs
    //digitalWrite(ERROR_LED_PIN, LOW);
    digitalWrite(SWBB_LED_PIN, LOW);
    digitalWrite(LUDP_LED_PIN, LOW);
    
    // Show a heartbeat blink if there has been no activity for a while
    if (mi_interval_elapsed(last_led_activity, 10000)) {
      digitalWrite(SWBB_LED_PIN, HIGH); // Green
      delay(100);
    }
  }

  void notification_function(NotificationType notification_type, const ModuleInterface *) {   
    if (notification_type == ntSampleOutputs) {
      uint8_t ix = 0;
      uint32_t elapsed_time = (uint32_t)(millis() - statistics_start);
      outputs.set_value(ix++, miGetUptime());
      outputs.set_value(ix++, elapsed_time);
      outputs.set_value(ix++, elapsed_time == 0 ? 0 : float(60000.0*packet_count[0])/elapsed_time);
      outputs.set_value(ix++, elapsed_time == 0 ? 0 : float(60000.0*packet_count[1])/elapsed_time);
      outputs.set_value(ix++, elapsed_time == 0 ? 0 : float(60000.0*byte_count[0])/elapsed_time);
      outputs.set_value(ix++, elapsed_time == 0 ? 0 : float(60000.0*byte_count[1])/elapsed_time);
      outputs.set_value(ix++, free_sockets);
      outputs.set_updated(); // Flag as completely updated
      restart_statistics();
    }
  }
};
