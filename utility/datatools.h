#pragma once

#include <cstdint>

inline uint32_t padValue(uint32_t value, uint32_t pad_size = 0x80){
  auto over = value % pad_size;
  return over ? value + pad_size - over : value;
}

struct ByteCounter{
  uint32_t offset;
  void addSize(uint32_t size) { offset += size; }
  inline uint32_t pad(uint32_t pad_size = 0x80) { return offset = padValue(offset, pad_size); }
};

struct ByteMap{
  uint32_t offset;
  uint32_t size;
  inline uint32_t endpoint() const { return offset + size; }
  inline uint32_t endpointPad(uint32_t pad_size = 0x80) const { return padValue(offset + size, pad_size); }
};
