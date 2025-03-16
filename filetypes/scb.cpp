#include "scb.h"

#include <utility/datatools.h>
#include <utility/streamtools.h>

#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>

namespace {
constexpr int32_t offset_unknown_header_data = 0x14;
constexpr int32_t offset_header_cache = 0x30;
constexpr int32_t offset_sections = 0x70;
// For some reason scb-file uses different paddings for different sections.
constexpr char pre_MSG_padding_literal = 0xCD;
constexpr char post_MSG_padding_literal = 0xCC;
// constexpr int32_t msg_offset = 148;
constexpr auto offset_data_size = 0x10;
} // namespace

namespace imas {
namespace file {

Manageable::Fileapi SCB::api() const {
  static auto const api = Fileapi{.base_extension = "scb",
                          .final_extension = "xlsx",
                          .type = ExtractType::file,
                          .signature = "Microsoft Excel Spreadsheet (*.xlsx)",
                          .extraction_title = "Extract strings as XLSX...",
                          .injection_title = "Import string as XLSX..."};
  return api;
}

MSG &SCB::msg_data() { return m_msg_data; }

Result SCB::extract(std::filesystem::path const &savepath) const {
#ifdef SCB_RESEARCH
  extractSections(savepath);
  return {true, ""};
#endif
  return m_msg_data.extract(savepath);
}

Result SCB::inject(std::filesystem::path const &openpath) {
  auto const res = m_msg_data.inject(openpath);
  if (res.first) {
    rebuild();
  }
  return res;
}

#ifdef SCB_RESEARCH
void SCB::extractSections(const std::filesystem::__cxx11::path& savepath) const
{
  for (auto section : m_sections_agg) {
    auto outpath = savepath;
    outpath.replace_extension(section->label);
    std::ofstream stream(outpath, std::ios_base::binary);
    stream.write(section->data.data(), section->data.size());
  }
}
#endif

Result SCB::openFromStream(std::basic_istream<char> *stream) {
  stream->seekg(offset_unknown_header_data);
  stream->read(m_header_cache.data(), m_header_cache.size());
  stream->seekg(offset_sections);

  for (auto section : m_sections_agg) {
    stream->read(section->label, sizeof(ScbSection::label));
    section->size = imas::utility::readLong(stream);
    section->offset = imas::utility::readLong(stream);
    stream->ignore(4);
  }
  std::ranges::sort(m_sections_agg, std::ranges::less{}, &ScbSection::offset);
  for (auto section : m_sections_agg) {
    stream->seekg(section->offset);
    section->data.resize(section->size);
    stream->read(section->data.data(), section->size);
  }
  m_msg_data.loadFromData(m_sections.MSG.data);
#ifdef SCB_RESEARCH
  m_lbn_data.loadFromData(m_sections.LBN.data);
  m_rsn_data.loadFromData(m_sections.RSN.data);
  m_vcn_data.loadFromData(m_sections.VCN.data);
#endif

  // Testing
  //updateSectionData();
  return {true, ""};
}

void SCB::rebuild() {
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
  while (next != end) {
    counter.addSize((*iter)->data.size());
    counter.pad(0x10);
    (*next)->offset = counter.offset;
    (*next)->size = (*next)->data.size();
    iter = next++;
  }
}

size_t SCB::size() const {
  ByteCounter counter{
      static_cast<uint32_t>(offset_sections + m_sections_agg.size() * 0x10)};
  for (auto section : m_sections_agg) {
    counter.addSize(section->size);
    counter.pad(0x10);
  }
  return counter.offset;
}

Result SCB::saveToStream(std::basic_ostream<char> *stream) {
  // Let's fill header
  stream->write("SCB", 3);
  utility::padStream(stream, 0, 8);
  stream->put(0x45);
  utility::padStream(stream, 0, 3);
  stream->put(0x45);
  int32_t const h_size = size() - 0x20;
  utility::writeLong(stream, h_size);
  stream->write(m_header_cache.data(), m_header_cache.size());
  // Writing section header
  for (auto section : m_sections_agg) {
    stream->write(section->label, sizeof(ScbSection::label));
    imas::utility::writeLong(stream, section->size);
    imas::utility::writeLong(stream, section->offset);
    imas::utility::padStream(stream, post_MSG_padding_literal, 4);
  }
  // Writing section data
  bool post_msg = false;
  for (auto section : m_sections_agg) {
    stream->write(section->data.data(), section->data.size());
    imas::utility::evenWriteStream(stream, post_msg ? post_MSG_padding_literal
                                                    : pre_MSG_padding_literal);
    if (section == &m_sections.MSG) {
      post_msg = true;
    }
  }
  // Finalizing
  utility::evenWriteStream(stream, post_MSG_padding_literal);
  return {true, ""};
}

} // namespace file
} // namespace imas
