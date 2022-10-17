#ifndef BNA_H
#define BNA_H

#include <QFile>
#include <fstream>

#include "bnacommontypes.h"

class BNA
{
public:
  BNA(){};
  void parseFile(const std::string &filename);

  bool valid() const;
  const std::vector<BNAFileEntry> &getFileData() const;
  const std::map<int, std::string> &getFolderLibrary() const;
  void extractFile(FileSignature signature, std::string out_path);
  void extractAll(std::string path);

private:
  void fetchAll();
  void extractFile(const BNAFileEntry &file_data, std::string out_path);

  bool m_valid = false;
  std::ifstream m_stream;
  std::map<int, std::string> m_folder_library;
  std::vector<BNAFileEntry> m_file_data;
};

#endif // BNA_H
