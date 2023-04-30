#include "about.h"
#include "ui_about.h"

AboutWindow::AboutWindow(QWidget *parent) :
                                QDialog(parent),
                                ui(new Ui::about)
{
  ui->setupUi(this);
}

AboutWindow::~AboutWindow()
{
  delete ui;
}
