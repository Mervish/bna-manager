#include "bna.h"

#include <QDebug>
#include <QtEndian>
#include <QDataStream>
#include <set>
#include "stdhacks.h"

namespace  {
template<class T>
inline void readToValue(std::ifstream &stream, T &value){
  stream.read((char *)&value, sizeof (value));
  value = byteswap(value);
}

auto constexpr terminator = '\0';

}

bool BNA::parseFile(std::string const& filename)
{
  m_stream.open(filename, std::ios_base::binary);
  if (!m_stream.is_open()) {    return false;   }
  m_file_data.clear();
  m_folder_library.clear();
  //Check idstring
  auto constexpr idsize = 4;
  char idstring[idsize];
  m_stream.read(idstring, idsize);

  if(memcmp("BNA0", idstring, idsize)){
    qInfo() << "The string is wrong!";
    return false;
  }

  //Get filecount
  int32_t files;
  readToValue(m_stream, files);

  std::set<int32_t> folder_offsets;

  //Parse header
  for(decltype(files) n_file = 0; n_file < files; ++n_file){
    BNAFileEntry entry;
    readToValue(m_stream, entry.offsets.dir_name.offset);
    readToValue(m_stream, entry.offsets.file_name.offset);
    readToValue(m_stream, entry.offsets.file_data.offset);
    readToValue(m_stream, entry.offsets.file_data.size);
    m_file_data.push_back(entry);
    folder_offsets.insert(entry.offsets.dir_name.offset);
  }
  //Get folder names
  for(auto offset: folder_offsets){
    std::stringbuf folderbuf;
    m_stream.seekg(offset);
    m_stream.get(folderbuf, terminator);
    m_folder_library[offset] = folderbuf.str();
  }
  //Get file names
  for(auto &entry: m_file_data){
    std::stringbuf folebuf;
    m_stream.seekg(entry.offsets.file_name.offset);
    m_stream.get(folebuf, terminator);
    entry.name = folebuf.str();
    entry.dir_name = m_folder_library[entry.offsets.dir_name.offset];
  }

  if(!m_file_data.empty()){
    return true;
  }
  return false;
}

const std::vector<BNAFileEntry> &BNA::getFileData() const
{
  return m_file_data;
}

const std::map<int, std::string> &BNA::getFolderLibrary() const
{
  return m_folder_library;
}

void BNA::extractFile(BNAFileEntry const& file, std::string const& out_path){
  std::ofstream ostream(out_path, std::ios_base::binary);
  if(file.loaded){
    ostream.write(file.data.data(), file.data.size());
  }else{
    std::vector<char> midbuf(file.offsets.file_data.size);
    m_stream.seekg(file.offsets.file_data.offset);
    m_stream.read(midbuf.data(), file.offsets.file_data.size);
    ostream.write(midbuf.data(), midbuf.size());
  }
}

void BNA::extractFile(FileSignature const& signature, std::string const& out_path)
{
  //It may look convoluted, because it is
  auto const off_it = std::ranges::find_if(m_folder_library, [folder = signature.path](auto const& pair){
    return folder == pair.second;
  });
  Q_ASSERT(off_it != m_folder_library.end());
  auto const file_it = std::ranges::find_if(m_file_data, [&signature, offset = off_it->first](auto const& file){
    return offset == file.offsets.dir_name.offset && signature.name == file.name;
  });
  Q_ASSERT(file_it != m_file_data.end());
  extractFile(*file_it, out_path);
}

void BNA::fetchAll(){
  for(auto &file_header: m_file_data){
    file_header.data.resize(file_header.offsets.file_data.size);
    m_stream.seekg(file_header.offsets.file_data.offset);
    m_stream.read(file_header.data.data(), file_header.offsets.file_data.size);
  }
  m_stream.close();
}

void BNA::extractAll(const std::string &path)
{
  for(auto &file: m_file_data){
    auto dirpath = path + "/" + m_folder_library.at(file.offsets.dir_name.offset);
    std::filesystem::create_directories(dirpath);
    extractFile(file, dirpath + "/" + file.name);
  }
}

