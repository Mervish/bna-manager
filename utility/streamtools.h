#pragma once

#include <cstdint>
#include <cstdlib>
#include <istream>
#include <vector>

namespace imas {
namespace utility {

template<class T>
inline void readToValue(std::basic_istream<char> *stream, T &value) {
  stream->read((char *)&value, sizeof (value));
  value = std::byteswap(value);
}

template<class T>
inline void writeFromValue(std::basic_ostream<char> *stream, T value) {
  value = std::byteswap(value);
  stream->write((char *)&value, sizeof(value));
}

inline int32_t readLong(std::basic_istream<char> *stream) {
  int32_t value;
  stream->read((char *)&value, sizeof(value));
  return std::byteswap(value);
}

inline int16_t readShort(std::basic_istream<char> *stream) {
  int16_t value;
  stream->read((char *)&value, sizeof(value));
  return std::byteswap(value);
}

inline uint16_t readUShort(std::basic_istream<char> *stream) {
  uint16_t value;
  stream->read((char *)&value, sizeof(value));
  return std::byteswap(value);
}

inline void writeLong(std::basic_ostream<char> *stream, int32_t value) {
  auto swapped = std::byteswap(value);
  stream->write((char *)&swapped, sizeof(swapped));
}

inline void writeShort(std::basic_ostream<char> *stream, int16_t value) {
  auto swapped = std::byteswap(value);
  stream->write((char *)&swapped, sizeof(swapped));
}

inline void padStream(std::basic_ostream<char> *stream, char pad_char, int pad_size) {
  std::vector<char> buf(pad_size, pad_char);
  stream->write(buf.data(), buf.size());
}

inline int evenReadStream(std::basic_istream<char> *stream, int pad_size = 0x10) {
  auto pos = stream->tellg();
  auto over = pos % pad_size;
  if (0 == over) {
    return 0;
  }
  auto left = pad_size - over;
  stream->ignore(left);
  return left;
}

inline void evenWriteStream(std::basic_ostream<char> *stream, char pad_char = 0, int pad_size = 0x10) {
  auto const pos = stream->tellp();
  auto const over = pos % pad_size;
  if (0 == over) {
    return;
  }
  auto const left = pad_size - over;
  std::vector<char> pad(left);
  std::fill(pad.begin(), pad.end(), pad_char);
  stream->write(pad.data(), pad.size());
}

} // namespace utility
} // namespace imas
