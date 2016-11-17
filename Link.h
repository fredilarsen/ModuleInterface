#pragma once

#include <PJON.h>

struct Link {
  virtual uint16_t receive() = 0;
  virtual uint16_t receive(uint32_t duration) = 0;
  
  virtual uint8_t update() = 0;
  virtual uint16_t send_packet(uint8_t id, const uint8_t *b_id, const char *string, uint16_t length, uint32_t timeout, uint16_t header) = 0;
  
  virtual const PacketInfo &get_last_packet_info() const = 0;
  virtual uint16_t get_header() const = 0;  
    
  virtual uint8_t get_id() const = 0;
  virtual const uint8_t *get_bus_id() const = 0;
  
  virtual void set_receiver(receiver r) = 0;
};