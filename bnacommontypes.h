#ifndef BNACOMMONTYPES_H
#define BNACOMMONTYPES_H

#include "utility/datatools.h"

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

struct FileSignature{
  std::string path;
  std::string name;
};

struct BNAExtractedFileData{
  std::string file_name;
  std::string dir_name;
  std::vector<char> file_data;
};

struct BNAOffsetData{
  ByteMap dir_name;
  ByteMap file_name;
  ByteMap file_data;
};

struct BNAFileEntry{
  BNAOffsetData offsets;
  std::string file_name;
  std::string_view dir_name;
  std::vector<char> file_data;
  bool loaded = false; //Indicates that file_data was loaded from the .bna file.
  //bool integrity = false; //Indicates that data is faulty
};

#endif // BNACOMMONTYPES_H
