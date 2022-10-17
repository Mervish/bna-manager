#ifndef BNA_H
#define BNA_H

#include <QFile>
#include <fstream>

#include "bnacommontypes.h"

class BNA
{
public:
  BNA(){};
  bool parseFile(std::string const& filename);

  const std::vector<BNAFileEntry> &getFileData() const;
  const std::map<int, std::string> &getFolderLibrary() const;
  void extractFile(FileSignature const& signature, std::string const& out_path);
  void extractAll(std::string const& path);

private:
  void fetchAll();
  void extractFile(const BNAFileEntry &file_data, std::string const& out_path);

  std::ifstream m_stream;
  std::map<int, std::string> m_folder_library;
  std::vector<BNAFileEntry> m_file_data;
};

#endif // BNA_H
