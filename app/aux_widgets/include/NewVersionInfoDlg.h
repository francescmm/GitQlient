#pragma once

#include <QDialog>

namespace Ui
{
class NewVersionInfoDlg;
}

class NewVersionInfoDlg : public QDialog
{
   Q_OBJECT

public:
   explicit NewVersionInfoDlg(QWidget *parent = nullptr);
   ~NewVersionInfoDlg();

private:
   Ui::NewVersionInfoDlg *ui;

   void goPreviousPage();
   void goNextPage();

   void createAddPage(const QString &title, const QStringList &imgsSrc, const QString &desc);
   void saveConfig();
};
