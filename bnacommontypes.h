#ifndef BNACOMMONTYPES_H
#define BNACOMMONTYPES_H

#include <string>
#include <vector>
#include <array>

enum class bnafiletype{
  nud,
  nut,
  skl,
  num,
  acc,
  other
};

inline int32_t padValue(int32_t value, int32_t pad_size = 0x80){
  auto over = value % pad_size;
  return over ? value + pad_size - over : value;
}

struct FileSignature{
  std::string path;
  std::string name;
};

struct BNAExtractedFileData{
  std::string file_name;
  std::string dir_name;
  std::vector<char> file_data;
};

struct ByteCounter{
  int32_t offset;
  void addSize(int32_t size) { offset += size; }
  inline int32_t pad(int32_t pad_size = 0x80) { return offset = padValue(offset); }
};

struct ByteMap{
  int32_t offset;
  int32_t size;
  inline int32_t endpoint() const { return offset + size; }
  inline int32_t endpointPad(int32_t pad_size = 0x80) const { return padValue(offset + size, pad_size); }
};

struct BNAOffsetData{
  ByteMap dir_name;
  ByteMap file_name;
  ByteMap file_data;
};

struct BNAFileEntry{
  BNAOffsetData offsets;
  std::string name;
  std::string_view dir_name;
  std::vector<char> data;
  bool loaded = false; //Indicates that file_data was loaded from the .bna file.
  //bool integrity = false; //Indicates that data is faulty
};

#endif // BNACOMMONTYPES_H
