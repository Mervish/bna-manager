#ifndef SCB_H
#define SCB_H

#include <filetypes/manageable.h>

#include <string>
#include <vector>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "msg.h"

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
  SCB();
  void loadFromData(std::vector<char> const &data) override;
  void loadFromFile(const std::string &filename) override;
  void saveToData(std::vector<char> &data) override;
  void saveToFile(const std::string &filename) override;
  void rebuild();
  MSG &msg_data();

  //virtuals
  virtual void extract(std::string const& savepath) override;
  virtual void inject(std::string const& openpath) override;

private:
  template<class S> void openFromStream(S &stream);
  template<class S> void saveToStream(S &stream);
  void updateSectionData();
  uint32_t headerSize() const;
  uint32_t calculateSize() const;

  std::array<char, 0x5C> m_header_cache;
  ScbData m_sections;
  std::vector<ScbSection *> m_sections_agg = {
      &m_sections.CMD, &m_sections.LBL, &m_sections.MSG, &m_sections.VCN,
      &m_sections.LBN, &m_sections.RSC, &m_sections.RSN};
  //Custom data parsers
  MSG m_msg_data;
};

} // namespace file
} // namespace imas

#endif // SCB_H
