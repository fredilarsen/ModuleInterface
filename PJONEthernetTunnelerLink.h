/* This PJONEthernetTunnelerLink class transfers PJON packets over TCP/IP Ethernet connections,
   assuming a PJONEthernetTunneler device receives the Ethernet packages and sends them ahead on a 
   PJON bus to the target device.   
*/

#pragma once

#include <PJONLink.h>
//#include <EthernetLink.h>

class PJONEthernetTunnelerLink : public Link {
  static void link_receiver_callback(uint8_t id, const uint8_t *payload, uint16_t length, void *callback_object) {
    if (callback_object && ((PJONEthernetTunnelerLink*)callback_object)->_receiver)
      ((PJONEthernetTunnelerLink*)callback_object)->_receiver((uint8_t*)payload, length, ((PJONEthernetTunnelerLink*)callback_object)->lpi);
  }
  receiver _receiver = NULL;
public:
  EthernetLink link;
  PacketInfo lpi;
  long dummy_bus_id = 0;
  
  PJONEthernetTunnelerLink() { link.set_receiver(link_receiver_callback, this); };
  PJONEthernetTunnelerLink(uint8_t id) { link. set_id(id); link.set_receiver(link_receiver_callback, this); };

  //**************** Overridden functions below *******************
  
  uint16_t receive() { return link.receive(); } 
  uint16_t receive(uint32_t duration) { return link.receive(duration); }
  
  uint8_t update() { link.update(); return 0; }
  
  uint16_t send_packet(uint8_t id, const uint8_t *b_id, const char *string, uint16_t length, uint32_t duration, uint16_t header) {
    // Create a PacketInfo struct, then serialize it along with the message
    memset(&lpi, 0, sizeof lpi);
// TODO: Use extended header and correct MI bit according to PJON standard    
    lpi.header = header==0 ? SENDER_INFO_BIT | ACK_REQUEST_BIT | MI_PJON_BIT : header; //MI_EXTENDED_HEADER_BIT;
//    lpi.extended_header = MI_PJON_BIT;
    lpi.receiver_id = id;
    copy_bus_id(lpi.receiver_bus_id, b_id);
    lpi.sender_id = get_id();
    copy_bus_id(lpi.sender_bus_id, (uint8_t*) &dummy_bus_id);
    
    // Now concatenate and send
    char buf[length + sizeof lpi];
    memcpy(buf, &lpi, sizeof lpi);
    memcpy(&buf[sizeof lpi], string, length);
    return link.send_with_duration(id, buf, sizeof lpi + length, 1000000);
  }
  
  const PacketInfo &get_last_packet_info() const { return lpi; }
  uint16_t get_header() const { return lpi.header; }  
      
  uint8_t get_id() const { return link.get_id(); }
  const uint8_t *get_bus_id() const { return (const uint8_t*) &dummy_bus_id; }
  
  void set_receiver(receiver r) { _receiver = r; }
//  TODO: PJONLink should accept link_receiver, and take the responsibility for handling PJON receiver and forwarding the call
  
};
  
