#pragma once

#include <MI_PJON/MILink.h>
#include <MI_PJON/PJONModuleInterface.h>

template<typename Strategy>
struct PJONLink : public MILink {
  PJON<Strategy> bus;

  PJONLink<Strategy>() { }
  PJONLink<Strategy>(uint8_t device_id) { bus.set_id(device_id); }
  PJONLink<Strategy>(const uint8_t *bus_id, uint8_t device_id) {
    bus.set_id(device_id); PJONTools::copy_bus_id(bus.bus_id, bus_id);
    if(!PJONTools::bus_id_equality(bus_id, bus.localhost)) bus.set_shared_network(true);
  }

  // These functions are required by the base class:

  uint16_t receive() { return bus.receive(); }
  uint16_t receive(uint32_t duration) { return bus.receive(duration); }

  uint8_t update() { return 0; /*bus.update();*/ }
  uint16_t send_packet(uint8_t id, const uint8_t *b_id, const char *string, uint16_t length, uint32_t timeout) {
    return bus.send_packet_blocking(id, b_id, (char *)string, length, bus.config | PJON_PORT_BIT, 0, MI_PJON_MODULE_INTERFACE_PORT, timeout);
  }

  const PJON_Packet_Info &get_last_packet_info() const { return bus.last_packet_info; }
  
  uint8_t get_id() const { return bus.device_id(); }
  const uint8_t *get_bus_id() const { return bus.bus_id; }

  void set_id(uint8_t id) { bus.set_id(id); }
  void set_bus_id(const uint8_t *bus_id) { PJONTools::copy_bus_id(bus.bus_id, bus_id); }
  
  void set_receiver(PJON_Receiver r, void *custom_pointer = NULL) {
    bus.set_receiver(r);
    bus.set_custom_pointer(custom_pointer);
  }
};
