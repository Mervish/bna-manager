#pragma once

#include <QFile>
#include <fstream>

#include "bnacommontypes.h"

namespace imas {
namespace file {

class BNA
{
public:
    BNA(){};
    bool loadFromFile(std::string const &filename);
    bool loadFromDir(std::string const &dirname);
    void saveToFile(std::string const &filename);
    BNAFileEntry& getFile(const FileSignature& signature);
    void reset();
    const std::vector<BNAFileEntry> &getFileData() const;
    std::vector<std::reference_wrapper<BNAFileEntry>> const getFiles(std::string const& extension);
    const std::map<int, std::string> &getFolderLibrary() const;
    void extractFile(FileSignature const& signature, std::string const& out_path);
    void replaceFile(FileSignature const& signature, std::string const& in_path);
    void extractAll(std::string const &path);
private:
    void fetchFile(BNAFileEntry& file);
    void fetchAll();
    void sortFileData();
    void extractFile(const BNAFileEntry &file_data, std::string const &out_path);
    void replaceFile(BNAFileEntry& file, std::string const& in_path);

    //data
    std::ifstream m_stream;
    std::map<int, std::string> m_folder_offset_library;
    std::vector<BNAFileEntry> m_file_data;
};

}
}
