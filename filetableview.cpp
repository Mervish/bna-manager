#include "filetableview.h"

#include <QDrag>
#include <QDragMoveEvent>
#include <QHeaderView>
#include <QMimeData>

FileTableView::FileTableView(QWidget *parent)
    : QTableView(parent)
{
    setAcceptDrops(true);
    m_extract_file_action = m_item_menu.addAction("Extract to...");
    m_replace_file_action = m_item_menu.addAction("Replace with...");
    m_item_menu.addSeparator();
    m_extract_action = m_item_menu.addAction("[debug]Extract...");
    m_inject_action = m_item_menu.addAction("[debug]Inject...");

    connect(m_extract_file_action, &QAction::triggered, [this]() {
        auto name = selectedIndexes().front().data().toString();
        emit extractionRequested(name);
    });
    connect(m_replace_file_action, &QAction::triggered, [this]() {
        auto name = selectedIndexes().front().data().toString();
        emit replacementRequested(name);
    });
}

void FileTableView::dragEnterEvent(QDragEnterEvent *event)
{
  if (event->mimeData()->hasFormat(fileMimeType())) {
    event->acceptProposedAction();
  }
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

void FileTableView::onItemMenu(QModelIndex const& index, QPoint const& pos) {
  auto const type = model()->data(index.siblingAtColumn(1)).toString().toStdString();
  if(auto const man = m_filetypes_managers->find(type); man != m_filetypes_managers->end()){
    auto const api = man->second->getApi();

    for(auto &connection: connections) {
        disconnect(connection);
    }
    connections.clear();

    m_extract_action->setText(QString::fromStdString(api.extraction_title));
    m_inject_action->setText(QString::fromStdString(api.injection_title));

    auto const name = selectedIndexes().front().data().toString();

    connections.push_back(connect(m_extract_action, &QAction::triggered, [this, name, extension = QString::fromStdString(api.base_extension)](){
        emit dataExtractionRequested(name, extension);
    }));
    connections.push_back(connect(m_inject_action, &QAction::triggered, [this, name, extension = QString::fromStdString(api.base_extension)](){
        emit dataInjectionRequested(name, extension);
    }));

    m_extract_action->setVisible(true);
    m_inject_action->setVisible(true);
  }else{
    m_extract_action->setVisible(false);
    m_inject_action->setVisible(false);
  }
  m_item_menu.exec(pos);
}

void FileTableView::contextMenuEvent(QContextMenuEvent *event)
{
  auto const pos = mapToGlobal(event->pos()) + QPoint(0, horizontalHeader()->height());
  auto const index = indexAt(event->pos());
  if (index.isValid()) {
    onItemMenu(index, pos);
  } else {
    m_table_menu.exec(pos);
  }
}

void FileTableView::setFiletypesManagers(imas::file::ManagerMap const* newFiletypes_managers)
{
  m_filetypes_managers = newFiletypes_managers;
}

