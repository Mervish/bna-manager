#include "bnasorter.h"

#include <QFile>

namespace  {
constexpr auto bna_sort_file = "bnastructdb.bin";
}

namespace imas {
namespace utility {

BNASorter::BNASorter() {
  QFile file(bna_sort_file);
  file.open(QIODevice::ReadOnly);
  QDataStream out(&file);
  out >> m_sort_data;
}

BNASorter::~BNASorter() {
  if (!m_changed) {
    return;
  }
  QFile file(bna_sort_file);
  file.open(QIODevice::WriteOnly);
  QDataStream out(&file);
  out << m_sort_data;
}





}
}
