#include "msg.h"
#include "utility/stringtools.h"

#include <ranges>
#include <utility/streamtools.h>

#include <numeric>

#include <OpenXLSX.hpp>

namespace {
constexpr char padding_literal = 0xCD;
constexpr auto offset_data_size = 0x10;
constexpr auto newline_literal = u'\n';
constexpr auto newline_string_literal = u"\n";
constexpr auto newline_string = u"\\n";

constexpr auto msg_encoding_flag = 0x1000000;

constexpr auto msg_export_column = 2;
constexpr auto msg_import_column = msg_export_column + 1;

template<typename CharT>
std::vector<std::basic_string_view<CharT>> split(std::basic_string_view<CharT> str, char delimiter) {
  std::vector<std::basic_string_view<CharT>> result;
  size_t start = 0;
  size_t end = str.find(delimiter);
  while (end != std::basic_string_view<CharT>::npos) {
    result.push_back(str.substr(start, end - start));
    start = end + 1;
    end = str.find(delimiter, start);
  }
  result.push_back(str.substr(start));
  return result;
}
} // namespace

namespace imas {
namespace file {

uint32_t MSG::headerSize() const {
  auto const entry_count = m_entries.size();
  return 16 + (entry_count % 2 ? (entry_count * 8) + 8 : (entry_count * 8));
}

uint32_t MSG::stringsSize() const {
  return std::accumulate(m_entries.begin(), m_entries.end(), 0,
                         [](const auto &a, const auto &b) {
                           return a + ((b.data.size() + 1) * 2);
                         });
}

size_t MSG::size() const {
  ByteCounter counter{32 + headerSize()};
  counter.pad(0x10);
  counter.addSize(stringsSize());
  counter.pad(0x10);
  return counter.offset;
}

Manageable::Fileapi MSG::api() const {
  static auto const api =
      Fileapi{.base_extension = "msg",
              .final_extension = "xlsx",
              .type = ExtractType::file,
              .signature = "Microsoft Excel Spreadsheet (*.XLSX)",
              .extraction_title = "Extract strings as XLSX...",
              .injection_title = "Import string from XLSX..."};
  return api;
}

Result MSG::extract(std::filesystem::path const &filepath) const {
  OpenXLSX::XLDocument doc;
  doc.create(filepath.string());

  auto wks = doc.workbook().worksheet("Sheet1");

  wks.row(1).values() =
      std::vector<std::string>{"name", "text", "translated", "note", "issues"};

  WidestringConv converter;

  for (auto const &[index, entry] : std::views::enumerate(m_entries)) {
    wks.cell(index + 2, msg_export_column).value() =
        converter.to_bytes(entry.data);
  }

  doc.save();
  return {true, ""};
}

Result MSG::inject(std::filesystem::path const &filepath) {
  OpenXLSX::XLDocument doc;
  doc.open(filepath.string());

  if (!doc.isOpen()) {
    return {false, "failed to open file"};
  }

  auto wks = doc.workbook().worksheet("Sheet1");

  WidestringConv converter;

  std::vector<std::u16string> new_strings;
  for (auto const &cell :
       wks.range(OpenXLSX::XLCellReference(2, msg_import_column),
                 OpenXLSX::XLCellReference(1 + m_entries.size(),
                                           msg_import_column))) {
    new_strings.emplace_back(converter.from_bytes(cell.getString()));
  }

  if (std::ranges::all_of(new_strings,
                          [](auto const &string) { return string.empty(); })) {
    return {false, "import column empty"};
  }

  if (std::ranges::equal(m_entries, new_strings, {}, &MSGEntry::data)) {
    return {false, "import data already present"};
  }

  for (auto [entry, string] : std::views::zip(m_entries, new_strings)) {
    entry.data = string;
  }

  return {true, ""};
}

Result MSG::openFromStream(std::basic_istream<char> *stream) {
  std::string label;
  label.resize(4);
  stream->read(label.data(), 4);
  stream->seekg(msg_count_offset);
  auto const count = imas::utility::readShort(stream);
  imas::utility::readToValue(stream, m_flags);
  m_entries.resize(count);
  stream->seekg(msg_header_offset);
  for (auto &entry : m_entries) {
    entry.map.size = imas::utility::readLong(stream);
    entry.map.offset = imas::utility::readLong(stream);
  }
  imas::utility::evenReadStream(stream);
  // msg uses offsets relative to the zero point.
  // Moreover, zero point is not defined in the header, so i'm just gonna assume
  // it goes straight after header
  size_t const zero_point = stream->tellg();
  bool const ansi = m_flags & msg_encoding_flag;
  for (auto &entry : m_entries) {
    stream->seekg(zero_point + entry.map.offset);
    // Let's strip terminating character from the string
    if (ansi) {
      entry.data.resize((entry.map.size - 1));
      for (auto &wide : entry.data) {
        stream->read((char *)&wide, 1);
      }
    } else {
      entry.data.resize((entry.map.size - 1) / 2);
      for (auto &wide : entry.data) {
        wide = imas::utility::readShort(stream);
      }
    }
  }
  return {true, ""};
}

Result MSG::saveToStream(std::basic_ostream<char>* stream) {
  // Let's fill header
  stream->write("MSG", 3);
  utility::padStream(stream, 0, 12);
  stream->put(0x45);
  utility::padStream(stream, 0, 16);
  // Write string count
  utility::writeFromValue(stream, m_entries.size()); // string count
  utility::writeFromValue(stream, m_flags);
  int16_t const str_size = stringsSize();
  utility::writeFromValue(stream, str_size); // string data size
  utility::writeFromValue(stream,
                      0x10); // idk what this does, seems to be always 0x10
  int16_t const header_size = headerSize();
  utility::writeFromValue(stream, header_size); // header size
  utility::padStream(stream, 0, 4);
  int32_t text_offset = 0;
  for (auto const &string : m_entries) {
    int32_t const str_size = (string.data.size() + 1) * 2;
    utility::writeFromValue(stream, str_size);
    utility::writeFromValue(stream, text_offset);
    text_offset += str_size;
  }
  utility::evenWriteStream(stream, padding_literal);
  for (auto const &string : m_entries) {
    for (auto const &wide : string.data) {
      utility::writeFromValue(stream, wide);
    }
    // Add terminating character to the string
    utility::writeFromValue(stream, wchar_t('\0'));
  }
  // Finalizing
  utility::evenWriteStream(stream, padding_literal);
  int32_t const size = int32_t(stream->tellp()) - 32;
  stream->seekp(offset_data_size);
  utility::writeFromValue(stream, size);
  return {true, ""};
}

} // namespace file
} // namespace imas
