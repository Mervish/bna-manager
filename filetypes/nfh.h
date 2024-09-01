#pragma once

#include "filetypes/manageable.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace imas {
namespace file {

struct CharData
{
  uint32_t value1;
  uint32_t sheet;
  uint16_t offset_x;
  uint16_t offset_y;
  uint16_t value4a;
  uint16_t value4b;
  uint16_t value5a;
  int16_t width;
  uint16_t value6a;
  uint16_t value6b;
  uint16_t value7a;
  char16_t character;
  uint16_t value8a;
  uint16_t value8b;
  void fromStream(std::basic_istream<char>* stream);
};

class NFH : public Manageable {
public:
  // Manageable interface
  virtual Fileapi api() const override;
  virtual Result extract(const std::filesystem::__cxx11::path& savepath) const override;
  virtual Result inject(const std::filesystem::__cxx11::path& openpath) override;

  std::vector<CharData> characters() const;

  protected:
  virtual Result openFromStream(std::basic_istream<char>* stream) override;
  virtual Result saveToStream(std::basic_ostream<char>* stream) override;
  virtual size_t size() const override;

private:
  std::string m_font_name;
  std::string m_font_type;
  uint32_t m_unknown1;
  uint32_t m_num_char_data;
  std::array<int32_t, 255> m_flags;
  std::vector<CharData> m_characters;
};

} // namespace file
} // namespace imas
