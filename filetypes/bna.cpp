#include "bna.h"

#include <QDebug>
#include <QMessageBox>

#include "utility/stdhacks.h"
#include "utility/streamtools.h"

#include <boost/range/adaptors.hpp>
#include <boost/range/combine.hpp>

#include <unordered_set>

namespace  {
template<class T>
inline void readToValue(std::ifstream &stream, T &value){
  stream.read((char *)&value, sizeof (value));
  value = byteswap(value);
}

auto constexpr terminator = '\0';
auto constexpr padding_char = 0;

#define FILETYPESTRING(a) \
{ #a, bnafiletype::a }

const std::map<std::string, bnafiletype> filetypemap{
    FILETYPESTRING(acc),
    FILETYPESTRING(nud),
    FILETYPESTRING(num),
    FILETYPESTRING(nut),
    FILETYPESTRING(skl)
};

bnafiletype getFileType(std::filesystem::path const& path){
  auto const res = filetypemap.find(path.extension().string().substr(1));
  return res == filetypemap.end() ? bnafiletype::other : res->second;
}

//For some reason, in BNA-files, if you have 2 pathes and one of them is prefix to another(i.e. "root/abc/efg" and "root/abc"),
//the prefix one("root/abc") is considered to be greater and goes after the longer path.
bool isBNASubfolder(std::string_view const& left, std::string_view const& right){
  if(left.size() == right.size()){
      return left < right;
  }
  auto const [mism_left, mism_right] = std::ranges::mismatch(left, right);
  auto const is_prefix = left.end() == mism_left || right.end() == mism_right;
  return is_prefix ? left.size() > right.size() : left < right;
}

//For some reason, even though file order based on the alphabet, it's not applied to the file extensions
bool isBNAFileOrder(std::string const& left, std::string const& right){
  auto leftPath = std::filesystem::path(left);
  auto rightPath = std::filesystem::path(right);
  if(leftPath.stem() == rightPath.stem()){
      auto typeleft  = getFileType(leftPath);
      auto typeright = getFileType(rightPath);
      return std::ranges::count(std::array{typeleft, typeright}, bnafiletype::other) ? left < right : typeleft < typeright;
  }
  return left < right;
}
}

namespace adaptor = boost::adaptors;

namespace imas {
namespace file {

//Emulates Namco's weird BNA-sorting pattern
void BNA::sortFileData()
{
  std::ranges::sort(m_file_data, [](BNAFileEntry const& left, BNAFileEntry const& right){
      return left.dir_name == right.dir_name ?
                 isBNAFileOrder(left.file_name, right.file_name) :
                 isBNASubfolder(left.dir_name, right.dir_name);
  });
}

bool BNA::loadFromFile(std::string const& filename)
{
  constexpr auto title = "Opening BNA file...";

  m_stream.open(filename, std::ios_base::binary);
  if (!m_stream.is_open()) {    
    QMessageBox::critical(nullptr, title, "Failed to open file!");
    return false;   
  }
  m_file_data.clear();
  m_folder_offset_library.clear();
  //Check idstring
  auto constexpr idsize = 4;
  char idstring[idsize];
  m_stream.read(idstring, idsize);

  if(memcmp("BNA0", idstring, idsize)){
    //warn user via message box
    QMessageBox::critical(nullptr, title, "This file is not a valid BNA file!");
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
    m_folder_offset_library[offset] = folderbuf.str();
  }
  //Get file names
  for(auto &entry: m_file_data){
    std::stringbuf folebuf;
    m_stream.seekg(entry.offsets.file_name.offset);
    m_stream.get(folebuf, terminator);
    entry.file_name = folebuf.str();
    entry.dir_name = m_folder_offset_library[entry.offsets.dir_name.offset];
  }

  if(!m_file_data.empty()){
    //sortFileData();
    return true;
  }
  return false;
}

bool BNA::loadFromDir(std::string const& dirname)
{
  std::filesystem::path root_path(dirname);
  std::unordered_map<std::string, std::vector<std::filesystem::directory_entry>> filemap;
  for (auto const &filepath : std::filesystem::recursive_directory_iterator(dirname)) {
    if (!filepath.is_regular_file()) {
        continue;
    }
    auto const dirpath = std::filesystem::relative(filepath.path().parent_path(), dirname).string();
    filemap[dirpath].push_back(filepath);
  }

  for(auto const& [index, pair]: filemap | adaptor::indexed(0))
  {
    auto const& [dir, file_list] = pair;
    auto fixed_dir(dir);
    std::ranges::replace(fixed_dir, '\\', '/');

    auto it_dir = m_folder_offset_library.insert({index, fixed_dir}).first;

    for(auto&& filename: file_list) {
        BNAFileEntry file_data{.file_name = filename.path().filename().string(),
                               .dir_name = it_dir->second,
                               .loaded = true};
        auto const size = std::filesystem::file_size(filename.path());
        file_data.file_data.resize(size);
        std::ifstream stream(filename.path().string(), std::ios_base::binary);
        stream.read(file_data.file_data.data(), size);
        m_file_data.emplace_back(std::move(file_data));
    }
  }

  sortFileData();

  return true;
}

void BNA::saveToFile(std::string const& filename)
{
  fetchAll();
  //constexpr auto title = "Saving BNA file...";

  std::ofstream stream(filename, std::ios_base::binary);
  if (!stream.is_open()) {    return;
  }
  //Let's begin calculating
  std::vector<BNAOffsetData> file_offset_data(m_file_data.size());
  auto file_combo = boost::combine(m_file_data, file_offset_data);
  ByteCounter byte_counter{ static_cast<uint32_t>(8 + (m_file_data.size() * 16)) };
  std::vector<char> namebuf;
  std::vector<std::string> directories;
  for(auto const& file: m_file_data){
    if(0 == std::ranges::count(directories, file.dir_name)){
        directories.emplace_back(file.dir_name);
    }
  }
  //dir offset calc
  for(auto const& dir: directories){
    ByteMap const cur_dir { byte_counter.offset, static_cast<uint32_t>(dir.size() + 1) };
    std::ranges::copy(dir, std::back_insert_iterator(namebuf));
    namebuf.push_back(0);
    byte_counter.addSize(cur_dir.size);
    for(auto const& [file_data, offset_data] : file_combo | boost::adaptors::filtered([dir](auto const& tuple){
                                                    auto const& [file_data, offset_data] = tuple;
                                                    return dir == file_data.dir_name;
                                                })){
        auto const size = file_data.file_name.size() + 1;
        std::ranges::copy(file_data.file_name, std::back_insert_iterator(namebuf));
        namebuf.push_back(0);
        offset_data.dir_name = cur_dir;
        offset_data.file_name = { byte_counter.offset, static_cast<uint32_t>(size) };
        byte_counter.addSize(size);
    }
  }
  //File data offset calc
  for(auto const& [file_data, offset_data]: file_combo){
    auto const size = file_data.file_data.size();
    offset_data.file_data = { byte_counter.pad(), static_cast<uint32_t>(size) };
    byte_counter.addSize(size);
  }
  //Let's start actual writing
  stream.write("BNA0", 4);

  imas::tools::writeLong(stream, m_file_data.size());
  for(auto const& file_offset : file_offset_data){
    imas::tools::writeLong(stream, file_offset.dir_name.offset);
    imas::tools::writeLong(stream, file_offset.file_name.offset);
    imas::tools::writeLong(stream, file_offset.file_data.offset);
    imas::tools::writeLong(stream, file_offset.file_data.size);
  }
  stream.write(namebuf.data(), namebuf.size());
  for(auto const& file_data : m_file_data){
    imas::tools::evenWriteStream(stream, padding_char, 0x80);
    stream.write(file_data.file_data.data(), file_data.file_data.size());
  }
}

const std::vector<BNAFileEntry> &BNA::getFileData() const
{
  return m_file_data;
}

std::vector<std::reference_wrapper<BNAFileEntry> > const BNA::getFiles(std::string const &extension)
{
  auto filtered = m_file_data | adaptor::filtered([extension](auto& entry){
    return entry.file_name.substr(entry.file_name.find_last_of('.') + 1) == extension;
                        }) | adaptor::transformed([](auto& entry) { return std::reference_wrapper(entry); });
  for(auto const& entry: filtered) {
    fetchFile(entry.get());
  }
  return std::vector<std::reference_wrapper<BNAFileEntry>>(filtered.begin(), filtered.end());
}

const std::map<int, std::string> &BNA::getFolderLibrary() const
{
  return m_folder_offset_library;
}

void BNA::extractFile(BNAFileEntry const& file, std::string const& out_path){
  std::ofstream ostream(out_path, std::ios_base::binary);
  if(file.loaded){
    ostream.write(file.file_data.data(), file.file_data.size());
  } else {
    std::vector<char> midbuf(file.offsets.file_data.size);
    m_stream.seekg(file.offsets.file_data.offset);
    m_stream.read(midbuf.data(), file.offsets.file_data.size);
    ostream.write(midbuf.data(), midbuf.size());
  }
}

void BNA::replaceFile(BNAFileEntry& file, std::string const& in_path){
  std::ifstream ifstream(in_path, std::ios_base::binary);
  if(!ifstream.is_open()){
    return;
  }
  //get file size
  auto const size = std::filesystem::file_size(in_path);
  file.file_data.resize(size);
  ifstream.read(file.file_data.data(), size);
  file.loaded = true;
}

void BNA::extractFile(FileSignature const& signature, std::string const& out_path)
{
  extractFile(getFile(signature), out_path);
}

void BNA::replaceFile(FileSignature const& signature, std::string const& in_path)
{
  replaceFile(getFile(signature), in_path);
}

BNAFileEntry& BNA::getFile(FileSignature const& signature)
{
  auto const off_it = std::ranges::find_if(m_folder_offset_library, [folder = signature.path](auto const& pair){
    return folder == pair.second;
  });
  Q_ASSERT(off_it != m_folder_offset_library.end());
  auto const file_it = std::ranges::find_if(m_file_data, [&signature, offset = off_it->first](auto const& file){
    return offset == file.offsets.dir_name.offset && signature.name == file.file_name;
  });
  Q_ASSERT(file_it != m_file_data.end());
  if(!file_it->loaded){
    fetchFile(*file_it);
  }
  return *file_it;
}

void BNA::reset()
{
  m_file_data.clear();
  m_folder_offset_library.clear();
  m_stream.close();
}

void BNA::fetchFile(BNAFileEntry& file) {
  if(!m_stream.is_open()) {
    return;
  }
  if(!file.loaded){
    file.file_data.resize(file.offsets.file_data.size);
    m_stream.seekg(file.offsets.file_data.offset);
    m_stream.read(file.file_data.data(), file.offsets.file_data.size);
    file.loaded = true;
  }
}

void BNA::fetchAll()
{
  //all loaded
  if(!m_stream.is_open()) {
    return;
  }
  for (auto &file_header :
       m_file_data | adaptor::filtered([](BNAFileEntry const &file) { return !file.loaded; })) {
    fetchFile(file_header);
  }
  m_stream.close();
}

void BNA::extractAll(const std::string &path)
{
  for(auto &file: m_file_data){
    auto dirpath = path + "/" + std::string(file.dir_name);
    std::filesystem::create_directories(dirpath);
    extractFile(file, dirpath + "/" + file.file_name);
  }
}
}
}

