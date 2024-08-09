
#include "pathsaver.h"

namespace {
constexpr auto savefile = "savedpath";
}

PathSaver::PathSaver(QObject *parent) : QObject{parent} {
  QFile file(savefile);
  file.open(QIODevice::ReadOnly);
  QDataStream out(&file);
  out >> m_path_data;
}

PathSaver::~PathSaver() {
  QFile file(savefile);
  file.open(QIODevice::WriteOnly);
  QDataStream out(&file);
  out << m_path_data;
}

bool PathSaver::interrogate(QFileDialog &dialog) {
  auto const filters = dialog.nameFilters();
  if(!filters.empty()) {
    dialog.setDirectory(m_path_data.value(filters.first(), ""));
  }else{
    dialog.setDirectory(m_dir_path);
  }
  auto const res = dialog.exec();
  if (!res) {
    return false;
  }
  auto const dir = dialog.directory();
  if(!filters.empty()) {
    m_path_data[filters.first()] = dir.absolutePath();
  }else{
    m_dir_path = dir.absolutePath();
  }
  return true;
}
