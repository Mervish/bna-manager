#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <filesystem>

namespace imas {
namespace file {

class Manageable
{
public:
    enum class extractType{
      file,
      dir
    };
    struct fileapi
    {
        std::string base_extension;
        std::string final_extension;
        extractType type = extractType::file;
        std::string signature;
        std::string extraction_title;
        std::string injection_title;
    };

    fileapi const getApi() const { return m_api; }
    //initialisation methods
    virtual void loadFromData(std::vector<char> const &data) = 0;
    virtual void loadFromFile(std::filesystem::path const& filename) = 0;
    virtual void saveToData(std::vector<char> &data) = 0;
    virtual void saveToFile(std::filesystem::path const& filename) = 0;
    //virtual methods
    virtual std::pair<bool, std::string> extract(std::filesystem::path const& savepath) = 0;
    virtual std::pair<bool, std::string> inject(std::filesystem::path const& openpath) = 0;

protected:
    fileapi m_api;
};

typedef std::map<std::string, std::unique_ptr<imas::file::Manageable>> ManagerMap;

} // namespace file
} // namespace imas
