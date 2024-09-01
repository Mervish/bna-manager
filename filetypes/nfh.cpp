#include "nfh.h"
#include "utility/streamtools.h"

namespace imas {
namespace file {
Manageable::Fileapi NFH::api() const{
  static auto const api = Fileapi{.base_extension = "nfh",
                          .final_extension = "json",
                          .type = ExtractType::file,
                          .signature = "JSON file (*.json)",
                          .extraction_title = "Export font data as JSON...",
                          .injection_title = "Import font data from JSON..."};
  return api;
}

Result NFH::extract(const std::filesystem::__cxx11::path& savepath) const
{
  return {true, ""};
}

Result NFH::inject(const std::filesystem::__cxx11::path& openpath)
{
  return {true, ""};
}

std::vector<CharData> NFH::characters() const
{
  return m_characters;
}

Result NFH::openFromStream(std::basic_istream<char>* stream)
{
  std::string label;
  label.resize(4);
  stream->read(label.data(), 4);

  if(std::string{'N', 'F', 'H', '\0'} != label) {
      return {false, "Wrong filetype? File label mismatch."};
  }

  m_unknown1 = imas::utility::readLong(stream);
  m_num_char_data = imas::utility::readLong(stream);
  imas::utility::readLong(stream);
  m_font_name.resize(0x20);
  stream->read(m_font_name.data(), 0x20);
  m_font_type.resize(0x20);
  stream->read(m_font_type.data(), 0x20);

  m_characters.resize(m_num_char_data);
  stream->read((char*)m_flags.data(), sizeof(int32_t) * 255);
  for(auto &character: m_characters) {
    character.fromStream(stream);
  }
  //stream->read((char*)m_characters.data(), sizeof(CharData) * m_num_char_data);

  return {true, ""};
}

Result NFH::saveToStream(std::basic_ostream<char>* stream)
{
  return {true, ""};
}

size_t NFH::size() const {
#warning placeholder
  return 0;
}

void CharData::fromStream(std::basic_istream<char>* stream) {
  imas::utility::readToValue(stream, value1   );
  imas::utility::readToValue(stream, sheet    );
  imas::utility::readToValue(stream, offset_x );
  imas::utility::readToValue(stream, offset_y );
  imas::utility::readToValue(stream, value4a  );
  imas::utility::readToValue(stream, value4b  );
  imas::utility::readToValue(stream, value5a  );
  imas::utility::readToValue(stream, width    );
  imas::utility::readToValue(stream, value6a  );
  imas::utility::readToValue(stream, value6b  );
  imas::utility::readToValue(stream, value7a  );
  imas::utility::readToValue(stream, character);
  imas::utility::readToValue(stream, value8a  );
  imas::utility::readToValue(stream, value8b  );
}

}
}
