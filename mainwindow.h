#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "about.h"
#include "filetypes/bna.h"
#include "filetablemodel.h"
#include "filetypes/manageable.h"
#include "utility/logger.h"

#include <QFileDialog>
#include <QMainWindow>
#include <QStandardItemModel>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

/* AGENDA
  1. [ ]Make BNA-reopenable. Add ability to close BNA-files.
  2. [?]Add ability to replace files in BNA.
  3. [ ]Add protection and checks to some functions.
  4. [*]Add console.
  5. [ ]Update file data in table when they are being updated.
  ? - not tested
  * - partial
 */

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private:
  void readFile(QString const &filename);
  void enableBNAActions(bool enable);

  Ui::MainWindow *ui;
  Logger* m_logger;
  imas::file::BNA bna;
  //Strings
  QString m_last_folder = "C:/Users/Mervish/Documents/Xenia/bna";
  QString const m_label_file_template;
  //Dialogs
  QFileDialog m_open_file_dialog;
  QFileDialog m_save_file_dialog;
  QFileDialog m_save_folder_dialog;
  //Misc windows
  AboutWindow m_about_window;
  //Models
  QStandardItemModel m_folder_tree_model;
  imas::model::FileTableModel m_file_table_model;
  //filetype manager
  template<class T>
  void registerManager();
  imas::file::ManagerMap m_filetypes_managers;
};
#endif // MAINWINDOW_H
