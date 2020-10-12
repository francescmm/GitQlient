#pragma once

#include <QDialog>

namespace Ui
{
class InitialRepoConfig;
}

class GitBase;

class InitialRepoConfig : public QDialog
{
   Q_OBJECT

public:
   explicit InitialRepoConfig(const QSharedPointer<GitBase> &git, QWidget *parent = nullptr);
   ~InitialRepoConfig();

private:
   Ui::InitialRepoConfig *ui;
   QSharedPointer<GitBase> mGit;
};
