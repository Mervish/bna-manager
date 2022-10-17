#include "filetableview.h"

#include <QDrag>
#include <QDragMoveEvent>
#include <QMimeData>

FileTableView::FileTableView(QWidget *parent) : QTableView(parent) {
  m_extract_action = m_item_menu.addAction("Extract to...");
  m_insert_action = m_table_menu.addAction("Add file...");

  connect(m_extract_action, &QAction::triggered, [this](){
    auto name = selectedIndexes().front().data().toString();
    emit extractionRequested(name);
  });
}

void FileTableView::dragEnterEvent(QDragEnterEvent *event)
{

}

void FileTableView::dragMoveEvent(QDragMoveEvent *event)
{
  if (event->mimeData()->hasFormat(fileMimeType())) {
    event->setDropAction(Qt::CopyAction);
    event->accept();
  } else {
    event->ignore();
  }
}

void FileTableView::dropEvent(QDropEvent *event)
{

}

void FileTableView::startDrag(Qt::DropActions supportedActions)
{
  QMimeData *mimeData = new QMimeData;
  mimeData->setData(fileMimeType(), "test_data");
  QDrag *drag = new QDrag(this);
  drag->setMimeData(mimeData);
  drag->exec(Qt::CopyAction);
}


