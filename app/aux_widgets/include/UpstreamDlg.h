#pragma once

#include <QDialog>

namespace Ui
{
class PullDlg;
}

class GitBase;

class UpstreamDlg : public QDialog
{
   Q_OBJECT

public:
   UpstreamDlg(QSharedPointer<GitBase> git, const QString &text, QWidget *parent = nullptr);
   ~UpstreamDlg() override;

   void accept() override;

private:
   Ui::PullDlg *ui;
   QSharedPointer<GitBase> mGit;
};
