#pragma once

#include <QDialog>

namespace Ui
{
class GitConfigDlg;
}

class Git;

class GitConfigDlg : public QDialog
{
   Q_OBJECT

public:
   explicit GitConfigDlg(const QSharedPointer<Git> &git, QWidget *parent = nullptr);
   ~GitConfigDlg() override;

private:
   Ui::GitConfigDlg *ui;
   QSharedPointer<Git> mGit;

   void accepted();
   void copyGlobalSettings(int checkState);
};
