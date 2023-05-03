#pragma once

#include <QAbstractTableModel>

namespace imas {
namespace model {

struct FileData{
    explicit FileData(QString filename, int size) : name(filename), size(size) {
        type = filename.right(3); //super hack!
    }
    QString name;
    QString type;
    int size;
};

struct FolderData{
  QString directory;
  std::vector<FileData> file_list;
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
  void clear();

  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  Qt::ItemFlags flags(const QModelIndex &index) const override{
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
  }

  void setFileData(QVariant folderData);
  QString const& currentDir() const { return m_folder_data.directory; };

private:
  FolderData m_folder_data;
};

}
}

Q_DECLARE_METATYPE(imas::model::FolderData);
