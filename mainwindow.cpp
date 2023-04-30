#include "mainwindow.h"

#include "about.h"
#include "filetypes/scb.h"

#include <QMessageBox>

#include <boost/range/adaptors.hpp>

#include "./ui_mainwindow.h"

namespace adaptor = boost::adaptors;

namespace  {

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

}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
      , ui(new Ui::MainWindow)
      , m_label_file_template("current file: %1")
      , m_open_file_dialog(this, "Select file to open", m_last_folder, bna_signature)
      , m_save_file_dialog(this, "Select where to save the file", m_last_folder, "All types (*.*)")
      , m_save_folder_dialog(this, "Select extraction folder", m_last_folder)
      , m_about_window(this)
{
  //filetypes managers
  registerManager<imas::file::SCB>();

  ui->setupUi(this);
  ui->folderTreeView->setModel(&m_folder_tree_model);
  ui->fileTableView->setModel(&m_file_table_model);
  ui->fileTableView->setFiletypesManagers(&m_filetypes_managers);
  ui->consoleView->setReadOnly(true);

  m_logger = Logger::getLogger();
  m_logger->setTextEdit(ui->consoleView);
  m_logger->log(LogMessageType::Info, "Testing logger.");

  //dialog modes
  m_open_file_dialog.setFileMode(QFileDialog::ExistingFile);
  m_save_file_dialog.setFileMode(QFileDialog::AnyFile);
  m_save_file_dialog.setAcceptMode(QFileDialog::AcceptSave);
  m_save_folder_dialog.setFileMode(QFileDialog::Directory);

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
  //open BNA
  connect(ui->actionOpenBna, &QAction::triggered, [this]{
    if(!m_open_file_dialog.exec()){      return;    }
    readFile(m_open_file_dialog.selectedFiles().first());
  });
  //save BNA-file as
  connect(ui->actionSave_as, &QAction::triggered, [this]{
    m_save_file_dialog.setNameFilter(bna_signature);
    if(!m_save_file_dialog.exec()){      return;    }
    bna.saveToFile(m_save_file_dialog.selectedFiles().first().toStdString());
    m_logger->log(LogMessageType::Info, "Saved as " + m_save_file_dialog.selectedFiles().first());
  });
  //pack directory into BNA
  connect(ui->actionOpenDir, &QAction::triggered, [this]{
    auto path = QFileDialog::getExistingDirectory(this, "Select root directory to pack", m_last_folder);
    auto save_path = QFileDialog::getSaveFileName(this, "Select file to save", m_last_folder, bna_signature);
    if(path.isEmpty() || save_path.isEmpty()){      return;    }
    imas::file::BNA packer;
    packer.loadFromDir(path.toStdString());
    packer.saveToFile(save_path.toStdString());
    m_logger->log(LogMessageType::Info, QString("Packed BNA to: %1").arg(save_path));
  });
  //extract all files from BNA
  connect(ui->actionExtract_all, &QAction::triggered, [this]{
    if(!m_save_folder_dialog.exec()){      return;    }
    auto const folder = m_save_folder_dialog.selectedFiles().first();
    bna.extractAll(folder.toStdString());
    m_logger->log(LogMessageType::Info, QString("Extracted BNA to: %1").arg(folder));
  });
  //table actions
  //extract file
  connect(ui->fileTableView, &FileTableView::extractionRequested, [this](QString const& filename){
    m_save_file_dialog.selectFile(filename);
    auto const type_signature = QString("Requested type (*.%1)").arg(filename.mid(filename.lastIndexOf('.') + 1));
    m_save_file_dialog.setNameFilter(type_signature);
    if(!m_save_file_dialog.exec()) { return; }
    bna.extractFile({m_file_table_model.currentDir().toStdString(), filename.toStdString()}, m_save_file_dialog.selectedFiles().first().toStdString());
    m_logger->log(LogMessageType::Info, QString("Extracted file: %1").arg(filename));
  });
  //replace file
  connect(ui->fileTableView, &FileTableView::replacementRequested, [this](QString const& filename){
    m_open_file_dialog.selectFile(filename);
    auto const type_signature = QString("Requested type (*.%1)").arg(filename.mid(filename.lastIndexOf('.') + 1));
    m_open_file_dialog.setNameFilter(type_signature);
    if(!m_open_file_dialog.exec()){      return;    }
    bna.replaceFile({m_file_table_model.currentDir().toStdString(), filename.toStdString()}, m_open_file_dialog.selectedFiles().first().toStdString());
    m_logger->log(LogMessageType::Info, QString("Replaced file: %1").arg(filename));
  });

  //manager dependand actions
  connect(ui->fileTableView, &FileTableView::dataExtractionRequested, [this](QString const& filename, QString const& key){
      auto &manager = m_filetypes_managers.at(key.toStdString());
      m_save_file_dialog.setNameFilter(QString::fromStdString(manager->getApi().signature));
      m_save_file_dialog.selectFile(filename.left(filename.lastIndexOf('.')));
      if(!m_save_file_dialog.exec()) { return; }
      auto const& file = bna.getFile({m_file_table_model.currentDir().toStdString(), filename.toStdString()});
      manager->loadFromData(file.file_data);
      manager->extract(m_save_file_dialog.selectedFiles().first().toStdString());
      //extraction is a simple one-way process
      m_logger->log(LogMessageType::Info, QString("Extracted data from %1 to %2").arg(filename, m_save_file_dialog.selectedFiles().first()));
  });
  connect(ui->fileTableView, &FileTableView::dataInjectionRequested, [this](QString const& filename, QString const& key){
      auto &manager = m_filetypes_managers.at(key.toStdString());
      m_open_file_dialog.setNameFilter(QString::fromStdString(manager->getApi().signature));
      m_open_file_dialog.selectFile(filename.left(filename.lastIndexOf('.')));
      if(!m_open_file_dialog.exec()) { return; }
      auto& file = bna.getFile({m_file_table_model.currentDir().toStdString(), filename.toStdString()});
      manager->loadFromData(file.file_data);
      manager->inject(m_open_file_dialog.selectedFiles().first().toStdString());
      manager->saveToData(file.file_data);
      m_logger->log(LogMessageType::Info, QString("Injected data from %1 to %2").arg(m_open_file_dialog.selectedFiles().first(), filename));
  });

#warning remove these
  //testing actions
  /*connect(ui->actionSCB_rebuilding_test, &QAction::triggered, [this]{
    //open the scb file
    auto path = QFileDialog::getOpenFileName(this, "Select scb file", m_last_folder, "SCB (*.scb)");
    if(path.isEmpty()){      return;    }
    imas::file::SCB scb;
    scb.loadFromFile(path.toStdString());
    scb.rebuild();
    //add suffix '_r' to path's filename
    auto const base_info = QFileInfo(path);
    auto const savefile_base_name = base_info.absolutePath() + '/' + base_info.baseName() + "_r.scb";
    scb.saveToFile(savefile_base_name.toStdString());
  });*/

  ui->FileLabel->setText(m_label_file_template.arg(text_file_label_none));
}

MainWindow::~MainWindow()
{
  delete ui;
}

void MainWindow::enableBNAActions(bool enable) {
  ui->actionExtract_all->setEnabled(enable);
  //ui->actionSave->setEnabled(enable);
  ui->actionSave_as->setEnabled(enable);
}

void MainWindow::readFile(const QString &filename)
{
  QFile file(filename);
  if(!file.open(QFile::ReadOnly)){
    QMessageBox::warning(this, "Error!", "Unable to open the file.");
    return;
  }
  if(!bna.loadFromFile(filename.toStdString())){
    QMessageBox::warning(this, "Error!", "Unable to parse .bna file.");
    return;
  }
  ui->FileLabel->setText(m_label_file_template.arg(filename));
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
    /*std::vector<NameSizePair> file_list;
    for(auto const& file: header | adaptor::filtered([off = off](BNAFileEntry const& entry){
                              return off == entry.offsets.dir_name.offset;
                            })){
      file_list.emplace_back(QString::fromStdString(file.file_name), file.offsets.file_data.size);
    }*/
    //Model initialisation should be inside the model.
    auto file_data_range = header | adaptor::filtered([off = off](BNAFileEntry const &entry) {
                               return off == entry.offsets.dir_name.offset;
                           })
                           | adaptor::transformed([](BNAFileEntry const &entry) {
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
  m_logger->log(LogMessageType::Info, QString("File loaded: %1").arg(filename));
}

//Warning: a black magic
template<class T>
void MainWindow::registerManager()
{
  auto manager = std::make_unique<T>();         //Create an instance of the type
  auto const key = manager->getApi().extension; //Get the key
  m_filetypes_managers[key] = std::unique_ptr<imas::file::Manageable>(manager.release()); //Upcast
}

