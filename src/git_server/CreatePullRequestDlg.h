#pragma once

#include <QDialog>

namespace Ui {
class CreatePullRequestDlg;
}

class CreatePullRequestDlg : public QDialog
{
   Q_OBJECT

public:
   explicit CreatePullRequestDlg(QWidget *parent = nullptr);
   ~CreatePullRequestDlg();

private:
   Ui::CreatePullRequestDlg *ui;
};

