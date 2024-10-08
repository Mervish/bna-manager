#pragma once

#include <utility/datatools.h>
#include <utility/result.h>

#include <filesystem>
#include <fstream>
#include <map>
#include <vector>

namespace imas {
namespace file {

struct BNAFileSignature{
  std::string path;
  std::string name;
};


struct BNAFileEntry{
  struct {
    int32_t dir_name;
    int32_t file_name;
    ByteMap file_data;
  }offsets;
  std::string file_name;
  std::string_view dir_name;
  std::vector<char> file_data;
  bool loaded = false; //Indicates that file_data was loaded from the .bna file.
  //bool integrity = false; //Indicates that data is faulty
  std::string getFullPath() const {
    return std::string(dir_name) + '/' + file_name;
  }
  BNAFileSignature getSignature() const {
    return {std::string(dir_name), file_name};
  }
};

class BNA
{
public:
  // working with files
  Result loadFromFile(std::filesystem::path const& filepath);
  void saveToFile(std::filesystem::path const& filepath);
  bool loadFromDir(std::filesystem::path const& dirpath);
  void extractAllToDir(std::filesystem::path const& dirpath);
  Result extractFile(BNAFileSignature const& signature,
                   std::filesystem::path const& out_path);
  Result replaceFile(BNAFileSignature const& signature,
                   std::filesystem::path const& in_path);
  //
  BNAFileEntry& getFile(const BNAFileSignature& signature);
  const std::vector<BNAFileEntry>& getFileData() const;
  std::vector<std::reference_wrapper<BNAFileEntry>> const
  getFiles(std::string const& extension);
  const std::map<int, std::string>& getFolderLibrary() const;

  void reset();

private:
  void fetchFile(BNAFileEntry& file);
  void fetchAll();
  void sortFileData();
  Result extractFile(const BNAFileEntry& file_data,
                   std::filesystem::path const& out_path);
  Result replaceFile(BNAFileEntry& file, std::filesystem::path const& in_path);
  //void generateFolderLibrary();

  // data
  std::filesystem::path m_filepath;                     //not necessary, but may be convenient to have
  std::ifstream m_read_stream;                          //Keeps the file opened, as long as we haven't read all of the data
  //std::map<std::string_view, std::vector<std::reference_wrapper<BNAFileEntry>>> m_folder_library;
  std::map<int, std::string> m_folder_offset_library;
  std::vector<BNAFileEntry> m_file_data;
};

}
}
