/* This CompositeLink class transfers PJON packets over one or more Links, having a routing table that specifies which link 
   must be used to reach a specific device. Devices which have not been registered will be reached through the default link.
   
   A concrete use for this class can be where most devices can be reached through a wire, but some must be reached through
   a wireless connection using. And perhaps some others that must be reached through Ethernet (TCP or UDP).
*/

#pragma once

#include <PJONLink.h>

// This define can be set to increase or decrease the max number of registered routes
#ifndef MAX_LINK_ROUTES
#define MAX_LINK_ROUTES 10
#endif

#define MAX_LINKS 5

class CompositeLink : public Link {
  Link *default_link = NULL;
  int8_t num_links = 0;
  Link *links[MAX_LINKS]; // A collection of all registered links in addition to the default
  
  uint8_t num_routes = 0;
  uint8_t device_ids[MAX_LINK_ROUTES];  // Each device's id...
  uint8_t link_ix[MAX_LINK_ROUTES];     // ...and the index in the links array of the link through which it can be reached

  PacketInfo lpi;
  long dummy_bus_id = 0;
  
  // Return array position of link if already registered, or -1 if not
  int8_t find_link(const Link* link) {
    for (int8_t i=0; i<num_links; i++) if (links[i] == link) return i;
    return -1;
  }
  
  // Find the link to be used for reaching a device
  Link *find_link_by_device(uint8_t device_id) {
    for (int8_t i=0; i<num_links; i++) if (device_ids[i] == device_id) return links[link_ix[i]];
    return default_link;
  }

  // Locate link or add it, then return is position in the array, -1 if not added.
  int8_t register_link(Link *link) {
    int8_t ix = find_link(link);
    if (ix < 0 && num_links < MAX_LINKS) { // Not present, must be added
      links[num_links++] = link;
      link->set_receiver(link_receiver_callback, this);
      return num_links-1;
    }
    return ix;
  }
  
  static void link_receiver_callback(uint8_t id, const uint8_t *payload, uint16_t length, void *callback_object) {
    if (callback_object && ((CompositeLink*)callback_object)->_receiver)
      ((CompositeLink*)callback_object)->_receiver((uint8_t*)payload, length, ((CompositeLink*)callback_object)->lpi);
  }
  receiver _receiver = NULL;
  
public:
  
  CompositeLink() {}

  // All devices not having an explicit association to a link will be reached through the default link.
  // Note that the default link MUST BE SET.
  void set_default_route(Link *link) { default_link = link; default_link.set_receiver(link_receiver_callback, this); }
  
  // Register that a specific device must be reached through a specific link
  bool add_route(uint8_t device_id, Link *link) { 
    if (num_devices >= MAX_LINK_ROUTES) return false;
    device_ids[num_devices] = device_id;
    link_ix[num_devices] = register_link(link);
    if (link_ix[num_devices] < 0) return false; // Could not add link -- too many?
    num_devices++;
    return true;
  }
  
  
  //**************** Overridden functions below *******************
  
  uint16_t receive() { 
    for (uint8_t i=0; i<num_links; i++) {
      uint16_t v = links[i].receive();
      if (v == ACK) return v;
    }
    return default_link->receive();
  }
  
  uint16_t receive(uint32_t duration) {
    uint16_t response;
    uint32_t time = micros();
    while((uint32_t)(micros() - time) <= duration) {
      response = receive();
      if(response == ACK) return ACK;
    }
    return response;
  };
  
  uint8_t update() {
    uint8_t num_packets = 0;    
    num_packets += default_link->update();
    for (uint8_t i=0; i<num_links; i++) num_packets += links[i]->update();
    return num_packets;
  }
  
  uint16_t send_packet(uint8_t id, const uint8_t *b_id, const char *string, uint16_t length, uint32_t duration, uint8_t header) {
    // Remember last packet info
    memset(&lpi, 0, sizeof lpi);
    lpi.header = header==0 ? SENDER_INFO_BIT | ACK_REQUEST_BIT | MI_PJON_BIT | MI_EXTENDED_HEADER_BIT : header;
    lpi.receiver_id = id;
    copy_bus_id(lpi.receiver_bus_id, b_id);
    lpi.sender_id = get_id();
    copy_bus_id(lpi.sender_bus_id, (uint8_t*) &dummy_bus_id);

    // Find link and ask it to send    
    Link *link = find_link_by_device(id);
    return link->send_packet(id, b_id, string, length, duration, header);
  }
  
  const PacketInfo &get_last_packet_info() const { return lpi; }
  uint16_t get_header() const { return lpi.header; }  
      
  uint8_t get_id() const { return link.get_id(); }
  const uint8_t *get_bus_id() const { return (const uint8_t*) &dummy_bus_id; }
  
  void set_receiver(receiver r) { _receiver = r; }
};
  
