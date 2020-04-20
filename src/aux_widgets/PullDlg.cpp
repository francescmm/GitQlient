#include "PullDlg.h"
#include "ui_PullDlg.h"

#include <QPushButton>

PullDlg::PullDlg(const QString &text, QWidget *parent)
   : QDialog(parent)
   , ui(new Ui::PullDlg)
{
   ui->setupUi(this);

   ui->lText->setText(text);
   ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Pull");
}

PullDlg::~PullDlg()
{
   delete ui;
}
