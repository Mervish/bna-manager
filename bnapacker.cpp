#include "bnapacker.h"

#include "utility/datatools.h"
#include "utility/stdhacks.h"
#include "utility/streamtools.h"

#include <QDebug>

#include <filesystem>
#include <fstream>
#include <map>
#include <ranges>


#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/combine.hpp>

namespace adaptor = boost::adaptors;



namespace  {
auto constexpr padding_char = 0;



inline void writeValueToStream(std::ofstream &stream, int32_t const& value){
  auto swapped = byteswap(value);
  stream.write((char *)&swapped, sizeof (swapped));
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
  ByteCounter byte_counter{ static_cast<uint32_t>(8 + (m_file_data.size() * 16)) };
  std::vector<char> namebuf;
  std::vector<std::string> directories;
  for(auto const& file: m_file_data){
    if(0 == std::ranges::count(directories, file.dir_name)){
      directories.push_back(file.dir_name);
    }
  }
  //dir offset calc
  for(auto const& dir: directories){
    ByteMap cur_dir { byte_counter.offset, static_cast<uint32_t>(dir.size() + 1) };
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
  writeValueToStream(stream, m_file_data.size());
  for(auto const& file_offset : file_offset_data){
    writeValueToStream(stream, file_offset.dir_name.offset);
    writeValueToStream(stream, file_offset.file_name.offset);
    writeValueToStream(stream, file_offset.file_data.offset);
    writeValueToStream(stream, file_offset.file_data.size);
  }
  stream.write(namebuf.data(), namebuf.size());
  for(auto const& file_data : m_file_data){
    imas::tools::evenWriteStream(stream, padding_char, 0x80);
    stream.write(file_data.file_data.data(), file_data.file_data.size());
  }
  return true;
}
