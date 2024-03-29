#include "msg.h"

#include <utility/streamtools.h>

#include <QJsonArray>
#warning "Remove debug linkage!"
#include <QFileInfo>

#include <fstream>

namespace  {
constexpr char padding_literal = 0xCD;
constexpr auto offset_data_size = 0x10;
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
    qInfo() << "File size = " << QFileInfo(QString::fromStdString(filename)).size();
    qInfo() << "Calculated size = " << calculateSize();
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

std::pair<bool, std::string> MSG::setJson(QJsonValue const &json) {
    auto const array = json.toArray();
    if (array.size() != m_entries.size()) {
        return {false, "MSG string injection error: Count mismatch. Expected " + std::to_string(m_entries.size()) + ", got " + std::to_string(array.size()) + ". Are you loading the correct file?"};
    }

    m_entries.clear();
    for (auto const &value : array) {
        m_entries.push_back({.data = value.toString().toStdU16String()});
    }
    return {true, ""};
}

QJsonArray MSG::getJson() {
    QJsonArray msg_json;
    for (auto const &msg : m_entries) {
        msg_json.append(QString::fromStdU16String(msg.data));
    }
    return msg_json;
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
