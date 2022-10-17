#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QFileDialog>
#include <QMainWindow>
#include <QStandardItemModel>

#include "bna.h"
#include "bnapacker.h"
#include "filetablemodel.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private:
  void readFile(QString const &filename);

  Ui::MainWindow *ui;
  BNA bna;
  //Strings
  QString m_last_folder;
  QString m_label_file_template;
  //Dialogs
  QFileDialog m_open_file_dialog;
  QFileDialog m_save_file_dialog;
  QFileDialog m_save_folder_dialog;
  //Models
  QStandardItemModel m_folder_tree_model;
  FileTableModel m_file_table_model;
};
#endif // MAINWINDOW_H
