#pragma once

#include <QString>
#include <QMap>
#include <ranges>

namespace imas {
namespace utility {

class BNASorter;

class FileSorter {
  friend BNASorter;
public:
  bool operator()(const std::string& left_path,
                  const std::string& right_path){
    return getScore(left_path) < getScore(right_path);
  }
private:
  FileSorter(QStringList const*const sort_data) : m_sort_data(sort_data) {};
  int getScore(std::string const& evaluated) {
    return m_sort_data->indexOf(evaluated.c_str());
  }
  QStringList const* m_sort_data;
};

class BNASorter {
public:
  static BNASorter* getSorter() {
    static BNASorter sorter;
    return &sorter;
  }
  FileSorter makeFileSorter(std::string const& bna_name) {
    return FileSorter(&m_sort_data[bna_name.c_str()]);
  }
  //check, if sorting has sense
  bool hasData(std::string const& bna_name) {
    return m_sort_data.contains(bna_name.c_str());
  }
  void setData(std::string const& bna_name, std::vector<std::string> const& file_list) {
    if(m_sort_data.contains(bna_name.c_str())) {
      return;
    }
    auto const qstring_list = std::views::transform(file_list, [](auto const& entry){
      return QString::fromStdString(entry);
    });
    m_sort_data[bna_name.c_str()] = QStringList(qstring_list.begin(), qstring_list.end());
    m_changed = true;
  }
private:
  BNASorter();
  ~BNASorter();
  bool m_changed = false;
  QMap<QString, QStringList> m_sort_data;
};

} // namespace utility
} // namespace imas
