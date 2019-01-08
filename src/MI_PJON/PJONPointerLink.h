#pragma once

#include <MI_PJON/MILink.h>
#include <MI_PJON/PJONModuleInterface.h>

template<typename Strategy>
struct PJONPointerLink : public MILink {
  PJON<Strategy> *bus_ptr = NULL;

  PJONPointerLink<Strategy>() { }
  PJONPointerLink<Strategy>(PJON<Strategy> &bus, uint8_t device_id) { this->bus_ptr = &bus; bus_ptr->set_id(device_id); }
  PJONPointerLink<Strategy>(PJON<Strategy> &bus, const uint8_t *bus_id, uint8_t device_id) {
    this->bus_ptr = &bus;
    bus_ptr->set_id(device_id); PJONTools::copy_bus_id(bus_ptr->bus_id, bus_id);
    if(!PJONTools::bus_id_equality(bus_id, bus_ptr->localhost)) bus_ptr->set_shared_network(true);
  }

  // Allow setting the bus
  void set_bus(PJON<Strategy> &bus) { bus_ptr = &bus; }
  void set_bus(PJON<Strategy> *bus_pointer) { bus_ptr = bus_pointer; }
  PJON<Strategy> *get_bus() { return bus_ptr; }

  // These functions are required by the base class:

  uint16_t receive() { return bus_ptr->receive(); }
  uint16_t receive(uint32_t duration) { return bus_ptr->receive(duration); }

  uint8_t update() { return 0; /*bus.update();*/ }
  uint16_t send_packet(uint8_t id, const uint8_t *b_id, const char *string, uint16_t length, uint32_t timeout) {
    return bus_ptr->send_packet_blocking(id, b_id, (char *)string, length, 
           bus_ptr->config | PJON_PORT_BIT, 0, MI_PJON_MODULE_INTERFACE_PORT, timeout);
  }

  const PJON_Packet_Info &get_last_packet_info() const { return bus_ptr->last_packet_info; }
  
  uint8_t get_id() const { return bus_ptr->device_id(); }
  const uint8_t *get_bus_id() const { return bus_ptr->bus_id; }

  void set_id(uint8_t id) { bus_ptr->set_id(id); }
  void set_bus_id(const uint8_t *bus_id) { PJONTools::copy_bus_id(bus_ptr->bus_id, bus_id); }
  
  void set_receiver(PJON_Receiver r, void *custom_pointer = NULL) {
    if (bus_ptr) {
      bus_ptr->set_receiver(r);
      bus_ptr->set_custom_pointer(custom_pointer);
    }
  }
};
