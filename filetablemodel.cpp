#include "filetablemodel.h"

#define SWITCH_TYPE_LITERAL(lit)\
case ImasFileType::lit:\
return QStringLiteral(#lit);

namespace  {

auto getTypeString(ImasFileType type){
  switch (type) {
  SWITCH_TYPE_LITERAL(acc)
  SWITCH_TYPE_LITERAL(nud)
  SWITCH_TYPE_LITERAL(num)
  SWITCH_TYPE_LITERAL(nut)
  SWITCH_TYPE_LITERAL(skl)
  }
  return QStringLiteral("u/n");
}

}

FileTableModel::FileTableModel(QObject *parent) : QAbstractTableModel(parent) {}

QVariant FileTableModel::headerData(int section, Qt::Orientation orientation,
                                    int role) const {
  if(Qt::Horizontal != orientation || role != Qt::DisplayRole){ return {}; }

  switch (section) {
  case 0:
    return "name";
  case 1:
    return "type";
  case 2:
    return "size";
  }
  return {};
}

int FileTableModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;

  return m_folder_data.file_list.size();
}

int FileTableModel::columnCount(const QModelIndex &parent) const {
  return 3;
}

QVariant FileTableModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
  {  return QVariant();   }
  if(role != Qt::DisplayRole)
  {  return QVariant();   }
  auto row = index.row();
  if(row >= m_folder_data.file_list.size()) { return{}; }

  switch (index.column()) {
  case 0:
    return m_folder_data.file_list[row].name;
  case 1:
    return {};
    //return getTypeString(m_file_list[row].type);
  case 2:
    return m_folder_data.file_list[row].size;
  default:
    return {};
  }

  return QVariant();
}

void FileTableModel::setFileData(QVariant folderData){
  emit beginResetModel();
  m_folder_data = folderData.value<FolderData>();
  emit endResetModel();
}
