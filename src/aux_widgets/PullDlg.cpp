#include "PullDlg.h"
#include "ui_PullDlg.h"

PullDlg::PullDlg(const QString &text, QWidget *parent)
   : QDialog(parent)
   , ui(new Ui::PullDlg)
{
   ui->setupUi(this);

   ui->lText->setText(text);
}

PullDlg::~PullDlg()
{
   delete ui;
}
