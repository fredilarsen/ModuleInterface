#pragma once

#include <MI_PJON/Link.h>

template<typename Strategy>
struct PJONLink : public Link {
  PJON<Strategy> bus;
  
  PJONLink<Strategy>(uint8_t device_id) { bus.set_id(device_id); }
  PJONLink<Strategy>(uint8_t device_id, const uint8_t *bus_id) {
    bus.set_id(device_id); copy_bus_id(bus.bus_id, bus_id);
  }
  
  // These functions are required by the base class:
  
  uint16_t receive() { return bus.receive(); }
  uint16_t receive(uint32_t duration) { return bus.receive(duration); }
  
  uint8_t update() { return 0; /*bus.update();*/ } 
  uint16_t send_packet(uint8_t id, const uint8_t *b_id, const char *string, uint16_t length, uint32_t timeout, uint16_t header) {
    return timeout == 0 ? bus.send_packet(id, b_id, (char *)string, length, header) 
      : bus.send_packet_blocking(id, b_id, (char *)string, length, header, timeout);  
  }
  
  const PJON_Packet_Info &get_last_packet_info() const { return bus.last_packet_info; } 
  uint16_t get_header() const { return bus.config; }
  uint8_t get_id() const { return bus.device_id(); }
  const uint8_t *get_bus_id() const { return bus.bus_id; }
  void set_receiver(PJON_Receiver r) { bus.set_receiver(r); }
};