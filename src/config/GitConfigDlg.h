#pragma once

#include <QDialog>

namespace Ui
{
class GitConfigDlg;
}

class GitBase;

class GitConfigDlg : public QDialog
{
   Q_OBJECT

public:
   explicit GitConfigDlg(const QSharedPointer<GitBase> &git, QWidget *parent = nullptr);
   ~GitConfigDlg() override;

private:
   Ui::GitConfigDlg *ui;
   QSharedPointer<GitBase> mGit;

   void accepted();
   void copyGlobalSettings(int checkState);
};
