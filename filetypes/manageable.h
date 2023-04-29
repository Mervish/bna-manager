
#ifndef ABSTRACTFILE_H
#define ABSTRACTFILE_H

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace imas {
namespace file {

class Manageable
{
public:
    struct fileapi
    {
        std::string extension;
        std::string signature;
        std::string extraction_title;
        std::string injection_title;
    };

    fileapi const getApi() const { return m_api; }
    //initialisation methods
    virtual void loadFromData(std::vector<char> const &data) = 0;
    virtual void loadFromFile(const std::string &filename) = 0;
    virtual void saveToData(std::vector<char> &data) = 0;
    virtual void saveToFile(const std::string &filename) = 0;
    //virtual methods
    virtual void extract(std::string const& savepath) = 0;
    virtual void inject(std::string const& openpath) = 0;

protected:
    fileapi m_api;
};

typedef std::map<std::string, std::unique_ptr<imas::file::Manageable>> ManagerMap;

} // namespace file
} // namespace imas

#endif // ABSTRACTFILE_H
