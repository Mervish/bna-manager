#include "msg.h"

#include <utility/streamtools.h>

#include <fstream>
#include <numeric>

namespace  {
constexpr char padding_literal = 0xCD;
constexpr auto offset_data_size = 0x10;
constexpr auto newline_literal = u'\n';
constexpr auto newline_string_literal = u"\n";
constexpr auto newline_string = u"\\n";

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

//find '\n' and replace them with "\n"
std::u16string convertNewlineCommand(std::u16string const& str) {
    std::u16string result;
    for (auto it = str.begin(); it != str.end(); ++it) {
        if (*it == newline_literal) {
            result += newline_string;
        } else {
            result += *it;
        }
    }
    return result;
}

//find "\n" and replace them with '\n'
std::u16string restoreNewlineCommand(std::u16string const& str) {
    std::u16string result(str);
    std::size_t found = result.find(newline_string);
    while (found != std::u16string::npos) {
        result.replace(found, 2, std::u16string(newline_string_literal));
        found = result.find(newline_string, 1);
    }
    return result;
}
}

namespace imas {
namespace file {
MSG::MSG() {}

void MSG::loadFromFile(const std::string &filename) {
    std::ifstream stream(filename, std::ios_base::binary);
    if (!stream.is_open()) {
        return;
    }
    openFromStream(stream);
}

void MSG::saveToFile(const std::string &filename)
{
    std::ofstream stream(filename, std::ios_base::binary);
    if(!stream.is_open()){
        return;
    }
    saveToStream(stream);
}

void MSG::loadFromData(const std::vector<char> &data) {
    boost::iostreams::array_source asource(data.data(), data.size());
    boost::iostreams::stream<boost::iostreams::array_source> data_stream{asource};
    openFromStream(data_stream);
}

uint32_t MSG::headerSize() const {
    auto const entry_count = m_entries.size();
    return 16 + (entry_count % 2 ? (entry_count * 8) + 8 : (entry_count * 8));
}

uint32_t MSG::stringsSize() const {
    return std::accumulate(m_entries.begin(), m_entries.end(), 0, [](const auto &a, const auto &b){
        return a + ((b.data.size() + 1) * 2);
    });
}

uint32_t MSG::calculateSize() const {
    ByteCounter counter{32 + headerSize()};
    counter.pad(0x10);
    counter.addSize(stringsSize());
    counter.pad(0x10);
    return counter.offset;
}

void MSG::saveToData(std::vector<char> &data) {
    data.resize(calculateSize());
    boost::iostreams::array_sink asink(data.data(), data.size());
    boost::iostreams::stream<boost::iostreams::array_sink> data_stream{asink};
    saveToStream(data_stream);
}

std::pair<bool, std::string> MSG::exportCSV(std::filesystem::path const& filepath) const{
    std::basic_ofstream<char16_t> stream(filepath);
    if(!stream.is_open()){
        return {false, "Failed to open file"};
    }
    stream << std::u16string(u"original;translated;translation notes;issues\n");
    for(auto const& entry : m_entries){
        stream << convertNewlineCommand(entry.data) << u";;;\n";
    }
    return {true, ""};
}

std::pair<bool, std::string> MSG::importCSV(std::filesystem::path const& filepath){
    std::basic_ifstream<char16_t> stream(filepath);
    if(!stream.is_open()){
        return {false, "Failed to open file"};
    }
    //skip the table header
    stream.ignore(std::numeric_limits<std::streamsize>::max(), u'\n');
    std::vector<std::u16string> lines;
    while (!stream.eof()) {
        std::u16string line;
        std::getline(stream, line, u'\n');
        lines.push_back(line);
    }
    //last line may be empty
    if(lines.back().empty()) {
      lines.resize(lines.size() - 1);
    }
    
    if(lines.size() != m_entries.size()) {
        return {false, "Failed to parse line. Wrong number of entries"};
    }

    m_entries.clear();
    for(auto const& line: lines)  {
        //we need second token
        int const token_begin_pos = line.find_first_of(';');
        int const token_end_pos = line.find_first_of(';', token_begin_pos + 1);
        if(token_end_pos == line.npos){
            return {false, "Failed to parse line. Wrong format"};
        }
        m_entries.push_back({.data = restoreNewlineCommand(line.substr(token_begin_pos + 1, token_end_pos - token_begin_pos - 1))});
    }

    return {true, ""};
}

template <class S> void MSG::openFromStream(S &stream) {
    std::string label;
    label.resize(4);
    stream.read(label.data(), 4);
    stream.seekg(msg_count_offset);
    auto count = imas::utility::readShort(stream);
    m_entries.resize(count);
    stream.seekg(msg_header_offset);
    for (auto &entry : m_entries) {
        entry.map.size = imas::utility::readLong(stream);
        entry.map.offset = imas::utility::readLong(stream);
    }
    imas::utility::evenReadStream(stream);
    // msg uses offsets relative to the zero point.
    // Moreover, zero point is not defined in the header, so i'm just gonna assume
    // it goes straight after header
    size_t const zero_point = stream.tellg();
    for (auto &entry : m_entries) {
        stream.seekg(zero_point + entry.map.offset);
        //Let's strip terminating character from the string
        entry.data.resize((entry.map.size - 1) / 2);
        for (auto &wide : entry.data) {
            wide = imas::utility::readShort(stream);
        }
    }
}

template<class S>
void MSG::saveToStream(S &stream)
{
    //Let's fill header
    stream.write("MSG", 3);
    utility::padStream(stream, 0, 12);
    stream.put(0x45);
    utility::padStream(stream, 0, 16);
    //Write string count
    utility::writeShort(stream, m_entries.size()); //string count
    utility::padStream(stream, 0, 4);
    int16_t const str_size = stringsSize();
    utility::writeShort(stream, str_size); //string data size
    utility::writeShort(stream, 0x10); //idk what this does
    int16_t const header_size = headerSize();
    utility::writeShort(stream, header_size); //header size
    utility::padStream(stream, 0, 4);
    int32_t text_offset = 0;
    for(auto const& string: m_entries){
        int32_t const str_size = (string.data.size() + 1) * 2;
        utility::writeLong(stream, str_size);
        utility::writeLong(stream, text_offset);
        text_offset += str_size;
    }
    utility::evenWriteStream(stream, padding_literal);
    for(auto const& string: m_entries){
        for (auto const &wide : string.data) {
            utility::writeShort(stream, wide);
        }
        //Add terminating character to the string
        utility::writeShort(stream, wchar_t('\0'));
    }
    //Finalizing
    utility::evenWriteStream(stream, padding_literal);
    int size = stream.tellp().operator long long() - 32;
    stream.seekp(offset_data_size);
    utility::writeLong(stream, size);
}

} // namespace file
} // namespace imas
