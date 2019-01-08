#pragma once

struct MILink {
  virtual uint16_t receive() = 0;
  virtual uint16_t receive(uint32_t duration) = 0;

  virtual uint8_t update() = 0;
  virtual uint16_t send_packet(uint8_t id, const uint8_t *b_id, const char *string, uint16_t length, uint32_t timeout) = 0;

  virtual const PJON_Packet_Info &get_last_packet_info() const = 0;

  virtual uint8_t get_id() const = 0;
  virtual const uint8_t *get_bus_id() const = 0;

  virtual void set_id(uint8_t id) = 0;
  virtual void set_bus_id(const uint8_t *bus_id) = 0;

  virtual void set_receiver(PJON_Receiver r, void *custom_ptr = NULL) = 0;
};
