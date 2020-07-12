#include "CreatePullRequestDlg.h"
#include "ui_CreatePullRequestDlg.h"

CreatePullRequestDlg::CreatePullRequestDlg(QWidget *parent) :
   QDialog(parent),
   ui(new Ui::CreatePullRequestDlg)
{
   ui->setupUi(this);
}

CreatePullRequestDlg::~CreatePullRequestDlg()
{
   delete ui;
}
