#include "mainwindow.h"

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
      , m_last_folder("C:/Users/Mervish/Documents/Xenia/bna")
      , m_label_file_template("current file: %1")
      , m_open_file_dialog(nullptr, "Select file to open", m_last_folder, bna_signature)
      , m_save_file_dialog(nullptr, "Select where to save file", m_last_folder, "All types (*.*)")
      , m_save_folder_dialog(nullptr, "Select extraction folder", m_last_folder)
{
  registerType<imas::file::SCB>();

  ui->setupUi(this);
  ui->folderTreeView->setModel(&m_folder_tree_model);
  ui->fileTableView->setModel(&m_file_table_model);
  ui->fileTableView->setFiletypesManagers(&m_filetypes_managers);

  m_open_file_dialog.setFileMode(QFileDialog::ExistingFile);
  m_save_file_dialog.setFileMode(QFileDialog::AnyFile);
  m_save_file_dialog.setAcceptMode(QFileDialog::AcceptSave);
  m_save_folder_dialog.setFileMode(QFileDialog::Directory);

  ui->actionOpenBna->setIcon(this->style()->standardIcon(QStyle::SP_DialogOpenButton));
  ui->actionOpenDir->setIcon(this->style()->standardIcon(QStyle::SP_DirOpenIcon));
  ui->actionExtract_all->setIcon(this->style()->standardIcon(QStyle::SP_DialogSaveAllButton));
  ui->actionSave->setIcon(this->style()->standardIcon(QStyle::SP_DialogSaveButton));
  ui->actionSave_as->setIcon(this->style()->standardIcon(QStyle::SP_DialogSaveButton));

  //set actions
  //open BNA-file
  connect(ui->actionOpenBna, &QAction::triggered, [this]{
    if(!m_open_file_dialog.exec()){      return;    }
    readFile(m_open_file_dialog.selectedFiles().first());
  });
  //pack directory into BNA
  connect(ui->actionOpenDir, &QAction::triggered, [this]{
    auto path = QFileDialog::getExistingDirectory(this, "Select root directory to pack", m_last_folder);
    auto save_path = QFileDialog::getSaveFileName(this, "Select file to save", m_last_folder, bna_signature);
    if(path.isEmpty() || save_path.isEmpty()){      return;    }
    imas::file::BNA packer;
    packer.loadFromDir(path.toStdString());
    packer.saveToFile(save_path.toStdString());
  });
  //extract all files from BNA
  connect(ui->actionExtract_all, &QAction::triggered, [this]{
    if(!m_save_folder_dialog.exec()){      return;    }
    bna.extractAll(m_save_folder_dialog.selectedFiles().first().toStdString());
  });
  //set model interaction
  connect(ui->folderTreeView->selectionModel(), &QItemSelectionModel::currentChanged, [this](const QModelIndex &current, const QModelIndex&){
    m_file_table_model.setFileData(current.data(file_list_role));
    ui->fileTableView->resizeColumnsToContents();
  });
  //set edit triggers
  connect(ui->fileTableView, &FileTableView::extractionRequested, [this](QString const& filename){
    m_save_file_dialog.selectFile(filename);
    if(!m_save_file_dialog.exec()) { return; }
    bna.extractFile({m_file_table_model.currentDir().toStdString(), filename.toStdString()}, m_save_file_dialog.selectedFiles().first().toStdString());
  });
  connect(ui->fileTableView, &FileTableView::dataExtractionRequested, [this](QString const& filename, QString const& key){
      auto &manager = m_filetypes_managers.at(key.toStdString());
      m_save_file_dialog.setNameFilter(QString::fromStdString(manager->getApi().signature));
      m_save_file_dialog.selectFile(filename.left(filename.lastIndexOf('.')));
      if(!m_save_file_dialog.exec()) { return; }
      auto const& file = bna.getFile({m_file_table_model.currentDir().toStdString(), filename.toStdString()});
      manager->loadFromData(file.file_data);
      manager->extract(m_save_file_dialog.selectedFiles().first().toStdString());
      //extraction is a simple one-way process
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
  });

  //save BNA-file
  connect(ui->actionSave, &QAction::triggered, [this]{
    if(!m_save_file_dialog.exec()){      return;    }
    bna.saveToFile(m_save_file_dialog.selectedFiles().first().toStdString());
  });

#warning remove these
  //testing actions
  connect(ui->actionSCB_rebuilding_test, &QAction::triggered, [this]{
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
  });

  ui->FileLabel->setText(m_label_file_template.arg(text_file_label_none));
}

MainWindow::~MainWindow()
{
  delete ui;
}

void MainWindow::enableBNAActions(bool enable) {
  ui->actionExtract_all->setEnabled(enable);
  ui->actionSave->setEnabled(enable);
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
}

//Warning: a black magic
template<class T>
void MainWindow::registerType()
{
  auto manager = std::make_unique<T>();         //Create an instance of the type
  auto const key = manager->getApi().extension; //Get the key
  m_filetypes_managers[key] = std::unique_ptr<imas::file::Manageable>(manager.release()); //Upcast
}

