#pragma once

// This class encapsulated an array of bytes, making sure it is freed when object goes out of scope

class BinaryBuffer {
private:  
  uint8_t *buffer = NULL;
  uint16_t len = 0;
public:
  BinaryBuffer() {}
  BinaryBuffer(const uint16_t length) { allocate(length); }
  ~BinaryBuffer() { deallocate(); }
  bool allocate(const uint16_t length) { 
    if (len < length) { 
      if (buffer) delete[] buffer;
      buffer = new uint8_t[length];
      len = length; 
    }
    return length == 0 || buffer != NULL;
  }
  void deallocate() { if (buffer) { delete[] buffer; buffer = NULL; len = 0; } }
  bool is_empty() const { return buffer == NULL; }
  
  const uint8_t *get() const { return buffer; }
  uint8_t *get() { return buffer; }
  
  const uint8_t operator [] (const uint16_t ix) const { return buffer[ix]; }
  uint8_t &operator [] (const uint16_t ix) { return buffer[ix]; }
  
  uint16_t length() const { return len; }
  
  void set_all(const uint8_t value) { for (uint16_t i=0; i<len; i++) buffer[i] = value; }
};