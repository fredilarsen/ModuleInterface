#pragma once

#include <Link.h>

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
      : send_packet_blocking(id, b_id, (char *)string, length, header, timeout, false);  
  }
  
  const PacketInfo &get_last_packet_info() const { return bus.last_packet_info; } 
  uint16_t get_header() const { return bus.get_header(); }
  uint8_t get_id() const { return bus.device_id(); }
  const uint8_t *get_bus_id() const { return bus.bus_id; }
  void set_receiver(receiver r) { bus.set_receiver(r); }
  
protected:
  // This function will block until packet has been delivered or timeout occurs.
  // It can accept incoming packets while in send back-off pauses if do_receive is set to true.
  uint16_t send_packet_blocking(
    uint8_t id,
    const uint8_t *b_id,
    const char *string,
    uint16_t length,
    uint16_t header = NOT_ASSIGNED,
    uint32_t timeout = 3000000,
    bool do_receive = false
  ) {
    if(!(length = bus.compose_packet(
      id,
      (uint8_t *) b_id,
      (char *)bus.data,
      string,
      length,
      header
    ))) return FAIL;
    uint16_t state = FAIL;
    uint32_t attempts = 0;
    uint32_t time = micros(), start = time;
    while(state != ACK && attempts <= MAX_ATTEMPTS && (uint32_t)(micros() - start) <= timeout) {
      state = bus.send_packet((char*)bus.data, length);
      if(state == ACK) return state;
      attempts++;
      if (do_receive) {
        uint32_t delay = (attempts * attempts * attempts);
        if (state != FAIL) delay += (uint32_t) (random(0, COLLISION_DELAY));
        while((uint32_t)(micros() - time) < delay) bus.receive();
      } else {
        if(state != FAIL) delayMicroseconds(random(0, COLLISION_DELAY));
        while((uint32_t)(micros() - time) < (attempts * attempts * attempts));
      }
      time = micros();
    }
    return state;
  } 
};