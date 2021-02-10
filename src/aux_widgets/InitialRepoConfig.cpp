#include "InitialRepoConfig.h"
#include "ui_InitialRepoConfig.h"

#include <GitQlientSettings.h>
#include <GitBase.h>

InitialRepoConfig::InitialRepoConfig(const QSharedPointer<GitBase> &git, QWidget *parent)
   : QDialog(parent)
   , ui(new Ui::InitialRepoConfig)
   , mGit(git)
{
   setAttribute(Qt::WA_DeleteOnClose);

   ui->setupUi(this);

   GitQlientSettings settings;

   ui->autoFetch->setValue(settings.localValue(mGit->getGitDir(), "AutoFetch", 5).toInt());
   ui->pruneOnFetch->setChecked(settings.localValue(mGit->getGitDir(), "PruneOnFetch", true).toBool());
   ui->updateOnPull->setChecked(settings.localValue(mGit->getGitDir(), "UpdateOnPull", false).toBool());
   ui->sbMaxCommits->setValue(settings.localValue(mGit->getGitDir(), "MaxCommits", 0).toInt());
}

InitialRepoConfig::~InitialRepoConfig()
{
   GitQlientSettings settings;
   settings.setLocalValue(mGit->getGitDir(), "AutoFetch", ui->autoFetch->value());
   settings.setLocalValue(mGit->getGitDir(), "PruneOnFetch", ui->pruneOnFetch->isChecked());
   settings.setLocalValue(mGit->getGitDir(), "UpdateOnPull", ui->updateOnPull->isChecked());
   settings.setLocalValue(mGit->getGitDir(), "MaxCommits", ui->sbMaxCommits->value());

   delete ui;
}
