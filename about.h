#pragma once

#include <QDialog>

namespace Ui {
class about;
}

class AboutWindow : public QDialog
{
  Q_OBJECT

      public:
  explicit AboutWindow(QWidget *parent = nullptr);
          ~AboutWindow();

private:
  Ui::about *ui;
};
