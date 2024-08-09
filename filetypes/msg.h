#pragma once

#include "filetypes/manageable.h"
#include "utility/datatools.h"

#include <filesystem>

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

class MSG : public Manageable {
    //MSG is not a real fuletype, but it uses pretty much all of the standard "Manageable" architecture
   public:
    Fileapi api() const override;
    Result extract(std::filesystem::path const& filename) const override;
    Result inject(std::filesystem::path const &csv) override;

   private:
    Result openFromStream(std::basic_istream<char> *stream) override;
    Result saveToStream(std::basic_ostream<char> *stream) override;
    std::vector<char> m_static_header;  // Data coming before string header
    std::vector<MSGEntry> m_entries;
    uint32_t headerSize() const;
    uint32_t stringsSize() const;
    size_t size() const override;
};

} // namespace file
} // namespace imas
