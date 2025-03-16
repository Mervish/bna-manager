#include "pathsaver.h"

#include "utility/logger.h"

namespace {
constexpr auto savefile = "savedpath";
constexpr int c_ver = 1;
}

PathSaver::PathSaver(QObject *parent) : QObject{parent} {
  QFile file(savefile);
  file.open(QIODevice::ReadOnly);
  int ver;
  QDataStream out(&file);
  out >> ver;
  if(ver == c_ver) {
    out >> m_dir_path;
    out >> m_path_data;
  }else{
    QFile::remove(savefile);
    Logger::getLogger()->info("Saved path file corrupted, resetting.");
  }
}

PathSaver::~PathSaver() {
  QFile file(savefile);
  file.open(QIODevice::WriteOnly);
  QDataStream out(&file);
  out << c_ver;
  out << m_dir_path;
  out << m_path_data;
}

bool PathSaver::interrogate(QFileDialog &dialog) {
  auto const filters = dialog.nameFilters();
  dialog.setDirectory(m_path_data.value(filters.first(), m_dir_path));
  auto const res = dialog.exec();
  if (!res) {
    return false;
  }
  auto const dir = dialog.directory().absolutePath();
  if(!filters.empty()) {
    m_path_data[filters.first()] = dir;
  }
  m_dir_path = dir;
  return true;
}
