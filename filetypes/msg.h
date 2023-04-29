#ifndef MSG_H
#define MSG_H

#include "utility/datatools.h"

#include <QJsonValue>

#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>

namespace imas {
namespace file {

struct MSGEntry{
    ByteMap map;
    std::u16string data;
};

constexpr int32_t msg_count_offset = 0x20;
constexpr int32_t msg_header_offset = 0x30;

class MSG
{
public:
    MSG();
    void loadFromData(std::vector<char> const& data);
    void loadFromFile(const std::string &filename);
    void saveToData(std::vector<char> &data);
    void saveToFile(const std::string &filename);
    QJsonArray getJson();
    bool fromJson(const QJsonValue &json);

private:
    template<class S> void openFromStream(S &stream);
    template<class S> void saveToStream(S &stream);
    std::vector<char> m_static_header; //Data coming before string header
    std::vector<MSGEntry> m_entries;
    uint32_t headerSize() const;
    uint32_t stringsSize() const;
    uint32_t calculateSize() const;
};

} // namespace file
} // namespace imas


#endif // MSG_H
