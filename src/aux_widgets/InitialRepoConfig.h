#pragma once

#include <QDialog>

namespace Ui
{
class InitialRepoConfig;
}

class GitQlientSettings;
class GitBase;

class InitialRepoConfig : public QDialog
{
   Q_OBJECT

public:
   explicit InitialRepoConfig(const QSharedPointer<GitBase> &git, const QSharedPointer<GitQlientSettings> &settings,
                              QWidget *parent = nullptr);
   ~InitialRepoConfig();

private:
   Ui::InitialRepoConfig *ui;
   QSharedPointer<GitQlientSettings> mSettings;
};
