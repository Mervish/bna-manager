#include "scb.h"

#include "utility/streamtools.h"
#include "utility/datatools.h"

#include <QDebug>

#include <QFile>
#include <filesystem>
#include <fstream>
#include <ranges>

#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>

namespace {
constexpr int32_t offset_unknown_header_data = 0x14;
constexpr int32_t offset_header_cache = 0x30;
constexpr int32_t offset_sections = 0x70;
//For some reason scb-file uses different paddings for different sections.
constexpr char pre_MSG_padding_literal = 0xCD;
constexpr char post_MSG_padding_literal = 0xCC;
//constexpr int32_t msg_offset = 148;
constexpr auto offset_data_size = 0x10;
} // namespace

namespace imas {
namespace file {

SCB::SCB()
{
    m_api = {.extension = "scb",
             .signature = "JSON file (*.json)",
             .extraction_title = "Extract strings...",
             .injection_title = "Insert string..."};
}

void SCB::loadFromFile(const std::string &filename) {
  std::ifstream stream(filename, std::ios_base::binary);
  if (!stream.is_open()) {
    return;
  }
  openFromStream(stream);
}

void SCB::saveToData(std::vector<char> &data)
{
  data.resize(calculateSize());
  boost::iostreams::array_sink asink(data.data(), data.size());
  boost::iostreams::stream<boost::iostreams::array_sink> data_stream{asink};
  saveToStream(data_stream);
}

void SCB::loadFromData(const std::vector<char> &data) {
  boost::iostreams::array_source asource(data.data(), data.size());
  boost::iostreams::stream<boost::iostreams::array_source> data_stream{asource};
  openFromStream(data_stream);
}

MSG &SCB::msg_data() { return m_msg_data; }

void SCB::extract(const std::string &savepath){
  auto const data = QJsonDocument(m_msg_data.getJson()).toJson();
  QFile file(QString::fromStdString(savepath));
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)){
    return;
  }

  file.write(data);
  file.close();
}

void SCB::inject(std::string const& openpath){
  QFile file(QString::fromStdString(openpath));
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return;
  }

  m_msg_data.fromJson(QJsonDocument::fromJson(file.readAll()).array());
  rebuild();
}

template <class S> void SCB::openFromStream(S &stream) {
  stream.seekg(offset_unknown_header_data);
  stream.read(m_header_cache.data(), m_header_cache.size());
  stream.seekg(offset_sections);

  for (auto section : m_sections_agg) {
    stream.read(section->label, sizeof(ScbSection::label));
    section->size = imas::tools::readLong(stream);
    section->offset = imas::tools::readLong(stream);
    stream.ignore(4);
  }
  std::ranges::sort(m_sections_agg, [](ScbSection *left, ScbSection *right) {
    return left->offset < right->offset;
  });
  for (auto section : m_sections_agg) {
    stream.seekg(section->offset);
    section->data.resize(section->size);
    stream.read(section->data.data(), section->size);
  }
  m_msg_data.loadFromData(m_sections.MSG.data);

  //Testing
  updateSectionData();
}

void SCB::saveToFile(const std::string &filename)
{
  std::ofstream stream(filename, std::ios_base::binary);
  if(!stream.is_open()){
    return;
  }
  saveToStream(stream);
}

void SCB::rebuild()
{
    msg_data().saveToData(m_sections.MSG.data);
    m_sections.MSG.size = 0;
    updateSectionData();
}

void SCB::updateSectionData() {
    /*if(std::ranges::all_of(m_sections_agg, [](ScbSection* section){
        return section->data.size() == section->size;
    })){
        return;
    }*/
    auto iter = m_sections_agg.begin();
    (*iter)->size = (*iter)->data.size();
    auto next = iter + 1;
    auto const end = m_sections_agg.cend();
    ByteCounter counter{m_sections_agg.front()->offset};
    while(next != end) {
        counter.addSize((*iter)->data.size());
        counter.pad(0x10);
        (*next)->offset = counter.offset;
        (*next)->size = (*next)->data.size();
        iter = next++;
    }
}

uint32_t SCB::headerSize() const
{
    return offset_sections + m_sections_agg.size() * 0x10;
}

uint32_t SCB::calculateSize() const
{
    ByteCounter counter { headerSize() };
    for(auto section: m_sections_agg) {
        counter.addSize(section->size);
        counter.pad(0x10);
    }
    return counter.offset;
}

template<class S>
void SCB::saveToStream(S &stream)
{
  //Let's fill header
  stream.write("SCB", 3);
  tools::padStream(stream, 0, 8);
  stream.put(0x45);
  tools::padStream(stream, 0, 3);
  stream.put(0x45);
  int32_t const size = calculateSize() - 0x20;
  tools::writeLong(stream, size);
  stream.write(m_header_cache.data(), m_header_cache.size());
  //Writing section header
  for(auto section: m_sections_agg) {
    stream.write(section->label, sizeof(ScbSection::label));
    imas::tools::writeLong(stream, section->size);
    imas::tools::writeLong(stream, section->offset);
    imas::tools::padStream(stream, post_MSG_padding_literal, 4);
  }
  //Writing section data
  bool post_msg = false;
  for(auto section: m_sections_agg) {
      stream.write(section->data.data(), section->data.size());
      imas::tools::evenWriteStream(stream, post_msg ? post_MSG_padding_literal : pre_MSG_padding_literal);
      if(section == &m_sections.MSG){
          post_msg = true;
      }
  }
  //Finalizing
  tools::evenWriteStream(stream, post_MSG_padding_literal);
}

} // namespace file
} // namespace imas
