#ifndef FILETABLEVIEW_H
#define FILETABLEVIEW_H

#include "filetypes/manageable.h"
#include <QContextMenuEvent>
#include <QMenu>
#include <QTableView>

class FileTableView : public QTableView {
  Q_OBJECT
public:
  FileTableView(QWidget *parent = nullptr);

  static QString fileMimeType() { return QStringLiteral("file"); }

  void setFiletypesManagers(const imas::file::ManagerMap* newFiletypes_managers);

  signals:
  void extractionRequested(QString const& filename);

  void dataExtractionRequested(QString const& filename, QString const& key);
  void dataInjectionRequested(QString const& filename, QString const& key);

protected:
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dragMoveEvent(QDragMoveEvent *event) override;
  void dropEvent(QDropEvent *event) override;
  void startDrag(Qt::DropActions supportedActions) override;
  void contextMenuEvent(QContextMenuEvent *event) override;

private:
  QMenu m_table_menu;
  QMenu m_item_menu;
  QAction *m_extract_file_action;
  QAction *m_replace_action;
  QAction *m_delete_action;
  //connections
  std::vector<QMetaObject::Connection> connections;
  //dynamic actions
  QAction *m_extract_action;
  QAction *m_inject_action;
  //filetype manager
  //(This is why thing const in rust by default)
  imas::file::ManagerMap const* m_filetypes_managers;

  void onItemMenu(QModelIndex const& index, QPoint const& pos);
};

#endif // FILETABLEVIEW_H
