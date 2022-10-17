#ifndef FILETABLEMODEL_H
#define FILETABLEMODEL_H

#include <QAbstractTableModel>

//Just to not use std::pair and have ability to add new params in the future
struct NameSizePair{
  QString name;
  int size;
};

struct FolderData{
  QString directory;
  std::vector<NameSizePair> file_list;
};

Q_DECLARE_METATYPE(FolderData);

enum class ImasFileType
{
  acc,
  nud,
  num,
  nut,
  skl
};

class FileTableModel : public QAbstractTableModel {
  Q_OBJECT

public:
  explicit FileTableModel(QObject *parent = nullptr);

  // Header:
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;

  // Basic functionality:
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;

  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  Qt::ItemFlags flags(const QModelIndex &index) const override{
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
  }

  void setFileData(QVariant folderData);
  QString const& currentDir() const { return m_folder_data.directory; };

private:
  struct FileData{
    QString name;
    //ImasFileType type;
    int size;
  };

  FolderData m_folder_data;
};

#endif // FILETABLEMODEL_H
