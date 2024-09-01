#include "mainwindow.h"

#include "about.h"
#include "filetypes/bxr.h"
#include "filetypes/nut.h"
#include "filetypes/scb.h"

#include <QMessageBox>
#include <QMimeData>

#include <boost/range/adaptors.hpp>

#include "./ui_mainwindow.h"
#include "utility/path.h"

namespace adaptor = boost::adaptors;

namespace {

auto constexpr file_list_role = Qt::UserRole + 1;
auto constexpr folder_path_role = Qt::UserRole + 2;
auto constexpr bna_signature = "Idolmaster archive (*.bna)";

auto constexpr text_file_label_none = "(none)";

std::optional<QStandardItem*> contains(QStandardItem *item, QString const& string){
  auto rows = item->rowCount();
  for(int row = 0; row < rows; ++row){
    auto candidate = item->child(row);
    if(string == candidate->data(Qt::DisplayRole)){
      return candidate;
    }
  }
  return {};
}

static QString const fileMimeType() { return QStringLiteral("file"); }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
      , ui(new Ui::MainWindow)
      , m_open_file_dialog(this, "Select file to open")
      , m_save_file_dialog(this, "Select where to save the file")
      , m_folder_dialog(this, "Select extraction folder")
      , m_about_window(this)
{
  //filetypes managers
  registerManager<imas::file::SCB>();
  registerManager<imas::file::BXR>();
  registerManager<imas::file::NUT>();

  ui->setupUi(this);
  setAcceptDrops(true);
  ui->folderTreeView->setModel(&m_folder_tree_model);
  ui->fileTableView->setModel(&m_file_table_model);
  ui->fileTableView->setFiletypesManagers(&m_filetypes_managers);
  ui->consoleView->setReadOnly(true);

  m_logger = Logger::getLogger();
  m_logger->setTextEdit(ui->consoleView);
  m_logger->log("Begin.");

  //dialog modes
  m_open_file_dialog.setFileMode(QFileDialog::ExistingFile);
  m_save_file_dialog.setFileMode(QFileDialog::AnyFile);
  m_save_file_dialog.setAcceptMode(QFileDialog::AcceptSave);
  m_folder_dialog.setFileMode(QFileDialog::Directory);
  m_folder_dialog.setNameFilter("folder");

  //icons
  ui->actionOpenBna->setIcon(this->style()->standardIcon(QStyle::SP_DialogOpenButton));
  ui->actionOpenDir->setIcon(this->style()->standardIcon(QStyle::SP_DirOpenIcon));
  ui->actionExtract_all->setIcon(this->style()->standardIcon(QStyle::SP_DialogSaveAllButton));
  ui->actionSave->setIcon(this->style()->standardIcon(QStyle::SP_DialogSaveButton));
  ui->actionSave_as->setIcon(this->style()->standardIcon(QStyle::SP_DialogSaveButton));

  //essential UI interaction
  connect(ui->folderTreeView->selectionModel(), &QItemSelectionModel::currentChanged, [this](const QModelIndex &current, const QModelIndex&){
    m_file_table_model.setFileData(current.data(file_list_role));
    ui->fileTableView->resizeColumnsToContents();
  });
  connect(ui->actionAbout, &QAction::triggered, [this]{
      m_about_window.exec();
  });

  //BNA actions
  connect(ui->actionOpenBna, &QAction::triggered, [this]{
    m_open_file_dialog.setNameFilter(bna_signature);
    if(!m_path_saver.interrogate(m_open_file_dialog)){      return;    }
    closeFile();
    openFile(m_open_file_dialog.selectedFiles().first());
  });
  connect(ui->actionClose, &QAction::triggered, [this]{
    closeFile();
    m_logger->info(QString("File %1 closed.").arg(m_current_file.string().c_str()));
    m_current_file.clear();
  });
  connect(ui->actionSave_as, &QAction::triggered, [this]{
    m_save_file_dialog.setNameFilter(bna_signature);
    if(!m_path_saver.interrogate(m_save_file_dialog)){      return;    }
    bna.saveToFile(m_save_file_dialog.selectedFiles().first().toStdString());
    m_logger->info("Saved as " + m_save_file_dialog.selectedFiles().first());
  });
  //pack directory into BNA
  connect(ui->actionOpenDir, &QAction::triggered, [this]{
    m_folder_dialog.setWindowTitle("Select the base directory to pack (the one containing the 'root')");
    if(!m_path_saver.interrogate(m_folder_dialog)){
        return;
    }
    std::filesystem::path const path = m_folder_dialog.selectedFiles().first().toStdString();
    if (imas::path::isEmptyRecursive(path)) {
        QMessageBox::warning(this,
                             "BNA packing",
                             QString("Directory %1 contains no files or subfiles.").arg(path.string().c_str()));
        return;
    }
    m_save_file_dialog.setNameFilter(bna_signature);
    m_save_file_dialog.selectFile(path.stem().string().c_str());
    if(!m_path_saver.interrogate(m_save_file_dialog)) { return; }
    imas::file::BNA packer;
    auto const save_path = m_save_file_dialog.selectedFiles().front();
    packer.loadFromDir(path);
    packer.saveToFile(save_path.toStdString());
    m_logger->info(QString("Packed BNA to: %1").arg(save_path));
  });
  //extract all files from BNA
  connect(ui->actionExtract_all, &QAction::triggered, [this] {
      m_folder_dialog.setWindowTitle("Select directory to create the dir with the BNA contents (it will be named after BNA-file)");
      if(!m_path_saver.interrogate(m_folder_dialog)){
          return;
      }
      auto const final_dir = m_folder_dialog.selectedFiles().first().toStdString() + '/'
                             + m_current_file.stem().string();
      if (std::filesystem::exists(final_dir)) {
          if (QMessageBox::No
              == QMessageBox::question(
                  this,
                  "BNA packing",
                  QString("Folder %1 already exists. Do you want to replace it's contents?")
                      .arg(QString::fromStdString(final_dir)))) {
              return;
          }
          std::filesystem::remove_all(final_dir);
      }
      bna.extractAllToDir(final_dir);
      m_logger->info(QString("Extracted BNA to: %1").arg(QString::fromStdString(final_dir)));
  });
  //table actions
  connect(ui->fileTableView, &FileTableView::extractionRequested, [this](QString const& filename){
    m_save_file_dialog.selectFile(filename);
    auto const type_signature = QString("Requested type (*.%1)").arg(filename.mid(filename.lastIndexOf('.') + 1));
    m_save_file_dialog.setNameFilter(type_signature);
    if(!m_path_saver.interrogate(m_save_file_dialog)) { return; }
    bna.extractFile({m_file_table_model.currentDir().toStdString(), filename.toStdString()}, m_save_file_dialog.selectedFiles().first().toStdString());
    m_logger->info(QString("Extracted file: %1").arg(filename));
  });
  connect(ui->fileTableView, &FileTableView::replacementRequested, [this](QString const& filename){
    m_open_file_dialog.selectFile(filename);
    auto const type_signature = QString("Requested type (*.%1)").arg(filename.mid(filename.lastIndexOf('.') + 1));
    m_open_file_dialog.setNameFilter(type_signature);
    if(!m_path_saver.interrogate(m_open_file_dialog)){      return;    }
    bna.replaceFile({m_file_table_model.currentDir().toStdString(), filename.toStdString()}, m_open_file_dialog.selectedFiles().first().toStdString());
    m_logger->info(QString("Replaced file: %1").arg(filename));
  });

  //manager dependand actions
  connect(ui->fileTableView, &FileTableView::dataExtractionRequested, [this](QString const& filename, QString const& key){
      auto &manager = m_filetypes_managers.at(key.toStdString());
      QString path;
      if(manager->api().type == imas::file::Manageable::ExtractType::file) {
        m_save_file_dialog.setNameFilter(QString::fromStdString(manager->api().signature));
        m_save_file_dialog.selectFile(filename.left(filename.lastIndexOf('.')));
        if(!m_path_saver.interrogate(m_save_file_dialog)) { return; }
        path = m_save_file_dialog.selectedFiles().first();
      }else{
        if(!m_path_saver.interrogate(m_folder_dialog)) { return; }
        path = m_folder_dialog.selectedFiles().first();
        path += '/' + filename.left(filename.lastIndexOf('.'));
      }
      auto const& file = bna.getFile({m_file_table_model.currentDir().toStdString(), filename.toStdString()});
      manager->loadFromData(file.file_data);
      if(auto const res = manager->extract(path.toStdString()); !res.first) {
          m_logger->error(QString::fromStdString(res.second));
          return;
      }
      m_logger->info(QString("Extracted data from %1 to %2").arg(filename, path));
  });
  connect(ui->fileTableView, &FileTableView::dataInjectionRequested, [this](QString const& filename, QString const& key){
      auto &manager = m_filetypes_managers.at(key.toStdString());
      QString path;
      if(manager->api().type == imas::file::Manageable::ExtractType::file) {
        m_open_file_dialog.setNameFilter(QString::fromStdString(manager->api().signature));
        m_open_file_dialog.selectFile(filename.left(filename.lastIndexOf('.')));
        if(!m_path_saver.interrogate(m_open_file_dialog)) { return; }
        path = m_open_file_dialog.selectedFiles().first();
      }else{
        if(!m_path_saver.interrogate(m_folder_dialog)) { return; }
        path = m_folder_dialog.selectedFiles().first() + '/' + filename.left(filename.lastIndexOf('.'));
      }
      auto& file = bna.getFile({m_file_table_model.currentDir().toStdString(), filename.toStdString()});
      manager->loadFromData(file.file_data);
      //failed injection should not modify the BNA
      if(auto const res = manager->inject(path.toStdString()); !res.first) {
          m_logger->error(QString::fromStdString(res.second));
          return;
      }
      manager->saveToData(file.file_data);
      m_logger->info(QString("Injected data from %1 to %2").arg(m_open_file_dialog.selectedFiles().first(), filename));
  });

#warning hide these in prod
  //debug actions
  connect(ui->actionSCB_rebuilding_test, &QAction::triggered, [this]{
    //open the scb file
    auto path_string = QFileDialog::getOpenFileName(this, "Select scb file", "", "SCB (*.scb)");
    if(path_string.isEmpty()){      return;    }
    auto const path = std::filesystem::path(path_string.toStdString());
    imas::file::SCB scb;
    scb.loadFromFile(path);
    scb.rebuild();
    scb.saveToFile(imas::path::applySuffix(path, "_r").string());
  });
  connect(ui->actionBXR_rebuilding_test, &QAction::triggered, [this]{
      auto path_string = QFileDialog::getOpenFileName(this, "Select scb file", "", "BXR (*.bxr)");
      if(path_string.isEmpty()){      return;    }
      imas::file::BXR bxr;
      auto const path = std::filesystem::path(path_string.toStdString());
      bxr.loadFromFile(path);
      //bxr.save(imas::path::applySuffix(path, "_r").string());
      bxr.extract(imas::path::changeExtension(path, "xml"));
  });

  setFilePathString(text_file_label_none);
}

MainWindow::~MainWindow()
{
  delete ui;
}

void MainWindow::enableBNAActions(bool enable) {
  ui->actionExtract_all->setEnabled(enable);
  //ui->actionSave->setEnabled(enable);
  ui->actionSave_as->setEnabled(enable);
  ui->actionClose->setEnabled(enable);
}

void MainWindow::setFilePathString(QString const& status) {
  ui->FileLabel->setText(QString("current file: %1").arg(status));
}

void MainWindow::openFile(const QString &filename)
{
  auto const [success, message] = bna.loadFromFile(filename.toStdString());
  if(!success){
    m_logger->error(message.c_str());
    return;
  }
  setFilePathString(filename);
  enableBNAActions(true);

  auto const& header = bna.getFileData();
  QVariant folder_folde_icon = this->style()->standardIcon(QStyle::SP_DirIcon);
  QVariant folder_files_icon = this->style()->standardIcon(QStyle::SP_DialogOpenButton);
  for(auto const& [off, folder]: bna.getFolderLibrary()){
    auto full_path = QString::fromStdString(folder);
    auto split_path = full_path.split('/');
    auto current_item = m_folder_tree_model.invisibleRootItem();
    for(auto const& folder: split_path){
      if(auto candidate = contains(current_item, folder)){
        current_item = candidate.value();
      }else{
        auto new_child = new QStandardItem(folder);
        new_child->setData(folder_folde_icon, Qt::DecorationRole);
        current_item->appendRow(new_child);
        current_item = new_child;
      }
    }
    //Let's set files, if any
    auto file_data_range = header | adaptor::filtered([off = off](imas::file::BNAFileEntry const &entry) {
                               return off == entry.offsets.dir_name;
                           })
                           | adaptor::transformed([](imas::file::BNAFileEntry const &entry) {
                                 return imas::model::FileData(QString::fromStdString(entry.file_name),
                                                 entry.offsets.file_data.size);
                             });
    std::vector file_list(file_data_range.begin(), file_data_range.end());
    if(!file_list.empty()){
      current_item->setData(QVariant::fromValue(imas::model::FolderData{QString::fromStdString(folder), file_list}), file_list_role);
      current_item->setData(folder_files_icon, Qt::DecorationRole);
    }
  }

  ui->folderTreeView->expandAll();
  m_current_file = filename.toStdString();
  m_logger->info(QString("File loaded: %1").arg(filename));
}

void MainWindow::closeFile()
{
  m_folder_tree_model.clear();
  m_file_table_model.clear();
  bna.reset();
  setFilePathString(text_file_label_none);
  enableBNAActions(false);
}

//Warning: a bit of black magic
template<class T>
void MainWindow::registerManager()
{
  auto manager = std::make_unique<T>();         //Create an instance of the type
  auto const key = manager->api().base_extension; //Get the key
  m_filetypes_managers[key] = std::unique_ptr<imas::file::Manageable>(manager.release()); //Upcast and put in map
}

bool checkType(QUrl const& url, QString const& ext) {
  //Check if url points to local file and file is of correct type
  if(!url.isLocalFile()){
    return false;
  }
  return url.toLocalFile().endsWith('.' + ext, Qt::CaseInsensitive);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
  if (event->mimeData()->hasUrls()
      && 1 == event->mimeData()->urls().size() && checkType(event->mimeData()->urls().front(), "bna")) {
    return event->acceptProposedAction();
  }
  event->ignore();
}

void MainWindow::dropEvent(QDropEvent *event)
{
  QUrl url = event->mimeData()->urls().front();
  QString filePath = url.toLocalFile();
  closeFile();
  openFile(filePath);

  // Accept the event
  event->acceptProposedAction();
}
