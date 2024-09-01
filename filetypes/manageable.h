#pragma once

#include <utility/result.h>

#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <spanstream>
#include <string>
#include <vector>

namespace imas {
namespace file {

class Manageable {
public:
  enum class ExtractType { file, dir };
  struct Fileapi {
    std::string base_extension;
    std::string final_extension;
    ExtractType type = ExtractType::file;
    std::string signature;
    std::string extraction_title;
    std::string injection_title;
  };

  virtual Fileapi api() const = 0;
  // initialisation methods
  Result loadFromData(std::vector<char> const &data) {
    std::ispanstream stream(data);
    return openFromStream(&stream);
  }
  Result loadFromFile(std::filesystem::path const &filename) {
    std::ifstream stream(filename, std::ios_base::binary);
    if (!stream.is_open()) {
      return {false, "Failed to open file " + filename.string()};
    }
    return openFromStream(&stream);
  }
  Result saveToData(std::vector<char> &data) {
    data.resize(size());
    std::ospanstream stream(data);
    return saveToStream(&stream);
  }
  Result saveToFile(std::filesystem::path const &filename) {
    std::ofstream stream(filename, std::ios_base::binary);
    if(!stream.is_open()){
      return {false, "Failed to create file " + filename.string()};
    }
    return saveToStream(&stream);
  }
  // virtual methods
  virtual Result
  extract(std::filesystem::path const &savepath) const = 0;
  virtual Result
  inject(std::filesystem::path const &openpath) = 0;
protected:
  virtual Result openFromStream(std::basic_istream<char> *stream) = 0;
  virtual Result saveToStream(std::basic_ostream<char> *stream) = 0;
  virtual size_t size() const = 0; //size of the output file
};

typedef std::map<std::string, std::unique_ptr<imas::file::Manageable>>
    ManagerMap;

} // namespace file
} // namespace imas
