#ifndef FILETABLEVIEW_H
#define FILETABLEVIEW_H

#include <QContextMenuEvent>
#include <QMenu>
#include <QTableView>

class FileTableView : public QTableView {
  Q_OBJECT
public:
  FileTableView(QWidget *parent = nullptr);

  static QString fileMimeType() { return QStringLiteral("file"); }

signals:
  void extractionRequested(QString const& filename);

protected:
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dragMoveEvent(QDragMoveEvent *event) override;
  void dropEvent(QDropEvent *event) override;
  void startDrag(Qt::DropActions supportedActions) override;
  void contextMenuEvent(QContextMenuEvent *event) override {
    auto const pos = mapToGlobal(event->pos());
    auto index = indexAt(event->pos());
    if (index.isValid()) {
      m_item_menu.exec(pos);
    } else {
      m_table_menu.exec(pos);
    }
  }

private:
  QMenu m_table_menu;
  QMenu m_item_menu;
  QAction *m_extract_action;
  QAction *m_replace_action;
  QAction *m_delete_action;
  QAction *m_insert_action;
};

#endif // FILETABLEVIEW_H
