#ifndef STREAMTOOLS_H
#define STREAMTOOLS_H

#include <fstream>
#include <vector>
#include <ranges>

namespace imas {
namespace tools {
  inline void readToValue(std::ifstream &stream, int32_t &value){
    stream.read((char *)&value, sizeof (value));
    value = _byteswap_ulong(value);
  }

  template<class S>
  inline int32_t readLong(S &stream){
    int32_t value;
    stream.read((char *)&value, sizeof (value));
    return _byteswap_ulong(value);
  }

  template<class S>
  inline uint32_t readULong(S &stream){
    uint32_t value;
    stream.read((char *)&value, sizeof (value));
    return _byteswap_ulong(value);
  }

  template<class S>
  inline int16_t readShort(S &stream){
    int16_t value;
    stream.read((char *)&value, sizeof (value));
    return _byteswap_ushort(value);
  }

  template<class S>
  inline void writeLong(S &stream, int32_t value){
    auto swapped = _byteswap_ulong(value);
    stream.write((char *)&swapped, sizeof (swapped));
  }

  template<class S>
  inline void writeShort(S &stream, int16_t value){
    auto swapped = _byteswap_ushort(value);
    stream.write((char *)&swapped, sizeof (swapped));
  }

  template<class S>
  inline void padStream(S &stream, char pad_char, int pad_size){
    std::vector<char> buf(pad_size, pad_char);
    stream.write(buf.data(), buf.size());
  }

  template<class S>
  inline int evenReadStream(S &stream, int pad_size = 0x10){
    auto pos = stream.tellg();
    auto over = pos % pad_size;
    if(0 == over) { return 0; }
    auto left = pad_size - over;
    stream.ignore(left);
    return left;
  }

  template<class S>
  inline void evenWriteStream(S &stream, char pad_char = 0, int pad_size = 0x10){
    auto pos = stream.tellp();
    auto over = pos % pad_size;
    if(0 == over) { return; }
    auto left = pad_size - over;
    std::vector<char> pad(left);
    std::fill(pad.begin(), pad.end(), pad_char);
    stream.write(pad.data(), pad.size());
  }
  
}
}

#endif // STREAMTOOLS_H
