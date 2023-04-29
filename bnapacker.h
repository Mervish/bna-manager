#ifndef BNAPACKER_H
#define BNAPACKER_H

#include <set>
#include <string>
#include <vector>

#include "bnacommontypes.h"

class BNAPacker
{
public:
  BNAPacker();
  void openDir(std::string path);
  bool saveBNA(std::string path);
private:
  std::vector<BNAExtractedFileData> m_file_data;
};

#endif // BNAPACKER_H
