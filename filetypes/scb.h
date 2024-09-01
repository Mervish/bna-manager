#pragma once

#include <filetypes/manageable.h>

#include <vector>
#include <filesystem>

#include "msg.h"

//#define SCB_RESEARCH

namespace imas {
namespace file {

struct ScbSection {
  uint32_t size = 0;
  uint32_t offset = 0;
  //Other modules should ignore size and offset, instead manipulating with 'data';
  char label[4];
  std::vector<char> data;
};

struct ScbData {
  ScbSection CMD;
  ScbSection LBL;
  ScbSection MSG;
  ScbSection VCN;
  ScbSection LBN;
  ScbSection RSC;
  ScbSection RSN;
};

class SCB : public Manageable {
public:
  Fileapi api() const override;
  void rebuild();
  MSG &msg_data();

  //virtuals
  virtual Result extract(std::filesystem::path const& savepath) const override;
  virtual Result inject(std::filesystem::path const& openpath) override;

#ifdef SCB_RESEARCH
  void extractSections(std::filesystem::path const& savepath) const;
#endif

private:
  Result openFromStream(std::basic_istream<char> *stream) override;
  Result saveToStream(std::basic_ostream<char> *stream) override;
  void updateSectionData();
  size_t size() const override;

  std::array<char, 0x5C> m_header_cache;
  ScbData m_sections;
  std::vector<ScbSection *> m_sections_agg = {
      &m_sections.CMD, &m_sections.LBL, &m_sections.MSG, &m_sections.VCN,
      &m_sections.LBN, &m_sections.RSC, &m_sections.RSN};
  //Custom data parsers
  MSG m_msg_data;
#ifdef SCB_RESEARCH
  MSG m_lbn_data;
  MSG m_vcn_data;
  MSG m_rsn_data;
#endif
};

} // namespace file
} // namespace imas
