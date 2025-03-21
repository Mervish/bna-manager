#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "about.h"
#include "filetypes/bna.h"
#include "filetablemodel.h"
#include "filetypes/manageable.h"
#include "utility/logger.h"
#include "utility/pathsaver.h"

#include <QFileDialog>
#include <QMainWindow>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

/* AGENDA
  1. [v]Make BNA-reopenable. Add ability to close BNA-files.
  2. [?]Add ability to replace files in BNA.
  3. [*]Add protection and checks to some functions.
  4. [*]Add console.
  5. [ ]Update file data in table when they are being updated.
  6. [*]Drag and drop support
  7. [*]File loading functions should output error messages along with the result.
  8. [ ]Make a BNA internal structure file order DB, so you could restore the original header even if you pack files from folders
  (as long as folder includes all of the original filenames)
  9. [ ]Add protection, if user tries to pack 'root' folder
  ? - not tested
  * - partial
  REFACTORING IDEAS
  1. Adopt QString throughout the project
  2. Rewrite BNA inner file management to use simple map<dirpath,filpath> system
  3. Adopt std::filesystem::path throughout the project
 */

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

protected:
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dropEvent(QDropEvent *event) override;

private:
  void openFile(QString const &filename);
  void closeFile();
  void enableBNAActions(bool enable);

  //interface helpers
  void setFilePathString(QString const& status);
  //bool processLoading(std::pair<bool, std::string> const& result) const;

  Ui::MainWindow *ui;
  Logger* m_logger;
  PathSaver m_path_saver;
  imas::file::BNA bna;
  //Strings
  std::filesystem::path m_current_file;
  //Dialogs
  QFileDialog m_open_file_dialog;
  QFileDialog m_save_file_dialog;
  QFileDialog m_folder_dialog;
  //Misc windows
  AboutWindow m_about_window;
  //Models
  QStandardItemModel m_folder_tree_model;
  imas::model::FileTableModel m_file_table_model;
  QSortFilterProxyModel m_filter_model;
  //filetype manager
  template<class T>
  void registerManager();
  imas::file::ManagerMap m_filetypes_managers;
};
#endif // MAINWINDOW_H
