#include "NewVersionInfoDlg.h"
#include "ui_NewVersionInfoDlg.h"

NewVersionInfoDlg::NewVersionInfoDlg(QWidget *parent)
   : QDialog(parent)
   , ui(new Ui::NewVersionInfoDlg)
{
   ui->setupUi(this);
   ui->pbClose->setVisible(false);
   ui->pbPrevious->setVisible(false);

   connect(ui->pbPrevious, &QPushButton::clicked, this, &NewVersionInfoDlg::goPreviousPage);
   connect(ui->pbNext, &QPushButton::clicked, this, &NewVersionInfoDlg::goNextPage);
   connect(ui->pbClose, &QPushButton::clicked, this, &QDialog::close);
}

NewVersionInfoDlg::~NewVersionInfoDlg()
{
   delete ui;
}

void NewVersionInfoDlg::goPreviousPage()
{
   if (ui->stackedWidget->currentIndex() == 1)
      ui->pbPrevious->setVisible(false);

   ui->pbClose->setVisible(false);
   ui->pbNext->setVisible(true);
}

void NewVersionInfoDlg::goNextPage()
{
   ui->pbPrevious->setVisible(true);
   ui->stackedWidget->setCurrentIndex(ui->stackedWidget->currentIndex() + 1);

   if (ui->stackedWidget->currentIndex() == ui->stackedWidget->count() - 1)
   {
      ui->pbNext->setVisible(false);
      ui->pbClose->setVisible(true);
   }
}
