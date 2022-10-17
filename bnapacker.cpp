#include "bnapacker.h"

#include <QDebug>
#include <filesystem>
#include <fstream>
#include <map>
#include <ranges>
#include "stdhacks.h"

#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/combine.hpp>

namespace ad = boost::adaptors;

#define FILETYPESTRING(a) \
{ #a, bnafiletype::a }

namespace  {
const std::map<std::string, bnafiletype> filetypemap{
  FILETYPESTRING(acc),
  FILETYPESTRING(nud),
  FILETYPESTRING(num),
  FILETYPESTRING(nut),
  FILETYPESTRING(skl)
};

inline void writeValueToStream(std::ofstream &stream, int32_t const& value){
  auto swapped = byteswap(value);
  stream.write((char *)&swapped, sizeof (swapped));
}

void padStream(std::ofstream &stream, size_t pad_size = 0x80){
  auto pos = stream.tellp();
  auto over = pos % pad_size;
  if(0 == over) { return; }
  auto left = pad_size - over;
  std::vector<char> padding(left);
  std::fill_n(padding.begin(), padding.size(), 0);
  stream.write(padding.data(), padding.size());
}

bnafiletype getFileType(std::filesystem::path const& path){
  auto const res = filetypemap.find(path.extension().string().substr(1));
  return res == filetypemap.end() ? bnafiletype::other : res->second;
}

//For some reason, in BNA-files, if you have 2 pathes and one of them is prefix to another(i.e. "root/abc/efg" and "root/abc"),
//the prefix one("root/abc") is considered to be greater and goes after the longer path.
bool isBNASubfolder(std::string const& left, std::string const& right){
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

BNAPacker::BNAPacker() {}

void BNAPacker::openDir(std::string path) {
  std::filesystem::path root_path(path);
  for (auto const &dir : std::filesystem::recursive_directory_iterator(path)) {
    if (!dir.is_regular_file()) {
      continue;
    }
    auto const filepath = dir.path().filename().string();
    auto const relative_dir =
        std::filesystem::relative(dir.path().parent_path(), path).string();
    BNAExtractedFileData file_data{.file_name = filepath,
                                   .dir_name = relative_dir};
    auto const size = std::filesystem::file_size(dir.path());
    file_data.file_data.resize(size);
    std::ifstream stream(dir.path().string(), std::ios_base::binary);
    stream.read(file_data.file_data.data(), size);
    m_file_data.emplace_back(std::move(file_data));
  }

  std::ranges::sort(m_file_data, [](BNAExtractedFileData const& left, BNAExtractedFileData const& right){
    return left.dir_name == right.dir_name ? isBNAFileOrder(left.file_name, right.file_name) : isBNASubfolder(left.dir_name, right.dir_name);
  });
  //Let's turn pathes into posix style
  for(auto &file: m_file_data){
    std::ranges::replace(file.dir_name, '\\', '/');
  }
}

bool BNAPacker::saveBNA(std::string path) {
  std::ofstream stream(path, std::ios_base::binary);
  if (!stream.is_open()) {    return false;   }
  //Let's begin calculating
  std::vector<BNAOffsetData> file_offset_data(m_file_data.size());
  auto file_combo = boost::combine(m_file_data, file_offset_data);
  ByteCounter byte_counter{ 8 + (static_cast<int32_t>(m_file_data.size()) * 16) };
  std::vector<char> namebuf;
  std::vector<std::string> directories;
  for(auto const& file: m_file_data){
    if(0 == std::ranges::count(directories, file.dir_name)){
      directories.push_back(file.dir_name);
    }
  }
  //dir offset calc
  for(auto dir: directories){
    ByteMap cur_dir { byte_counter.offset , static_cast<int32_t>(dir.size()) + 1 };
    std::ranges::copy(dir, std::back_insert_iterator(namebuf));
    namebuf.push_back(0);
    byte_counter.addSize(cur_dir.size);
    for(auto const& [file_data, offset_data] : file_combo | boost::adaptors::filtered([dir](auto const& tuple){
                                                  auto const& [file_data, offset_data] = tuple;
                                                  return dir == file_data.dir_name;
                                                })){
      auto size = file_data.file_name.size() + 1;
      std::ranges::copy(file_data.file_name, std::back_insert_iterator(namebuf));
      namebuf.push_back(0);
      offset_data.dir_name = cur_dir;
      offset_data.file_name = { byte_counter.offset, static_cast<int32_t>(size) };
      byte_counter.addSize(size);
    }
  }
  //File data offset calc
  for(auto const& [file_data, offset_data]: file_combo){
    auto size = file_data.file_data.size();
    offset_data.file_data = { byte_counter.pad(), static_cast<int32_t>(size) };
    byte_counter.addSize(size);
  }
  //Let's start actual writing
  stream.write("BNA0", 4);
  writeValueToStream(stream, m_file_data.size());
  for(auto const& file_offset : file_offset_data){
    writeValueToStream(stream, file_offset.dir_name.offset);
    writeValueToStream(stream, file_offset.file_name.offset);
    writeValueToStream(stream, file_offset.file_data.offset);
    writeValueToStream(stream, file_offset.file_data.size);
  }
  stream.write(namebuf.data(), namebuf.size());
  for(auto const& file_data : m_file_data){
    padStream(stream);
    stream.write(file_data.file_data.data(), file_data.file_data.size());
  }
  return true;
}
