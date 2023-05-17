#pragma once

#include "utility/datatools.h"
#include <QFile>
#include <fstream>

namespace imas {
namespace file {

struct BNAFileSignature{
  std::string path;
  std::string name;
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

class BNA
{
public:
    BNA(){};
    bool loadFromFile(std::filesystem::path const& filepath);
    void saveToFile(std::filesystem::path const& filepath);
    bool loadFromDir(std::filesystem::path const& dirpath);
    void extractAll(std::filesystem::path const& dirpath);
    BNAFileEntry& getFile(const BNAFileSignature& signature);
    const std::vector<BNAFileEntry> &getFileData() const;
    std::vector<std::reference_wrapper<BNAFileEntry>> const getFiles(std::string const& extension);
    const std::map<int, std::string> &getFolderLibrary() const;
    void extractFile(BNAFileSignature const& signature, std::filesystem::path const& out_path);
    void replaceFile(BNAFileSignature const& signature, std::string const& in_path);
    void reset();
  private:
    void fetchFile(BNAFileEntry& file);
    void fetchAll();
    void sortFileData();
    void extractFile(const BNAFileEntry &file_data, std::filesystem::path const& out_path);
    void replaceFile(BNAFileEntry& file, std::filesystem::path const& in_path);

    //data
    std::ifstream m_stream;
    std::map<int, std::string> m_folder_offset_library;
    std::vector<BNAFileEntry> m_file_data;
};

}
}
