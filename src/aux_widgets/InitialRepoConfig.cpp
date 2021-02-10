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

   GitQlientSettings settings(mGit->getGitDir());

   ui->autoFetch->setValue(settings.localValue("AutoFetch", 5).toInt());
   ui->pruneOnFetch->setChecked(settings.localValue("PruneOnFetch", true).toBool());
   ui->updateOnPull->setChecked(settings.localValue("UpdateOnPull", false).toBool());
   ui->sbMaxCommits->setValue(settings.localValue("MaxCommits", 0).toInt());
}

InitialRepoConfig::~InitialRepoConfig()
{
   GitQlientSettings settings(mGit->getGitDir());
   settings.setLocalValue("AutoFetch", ui->autoFetch->value());
   settings.setLocalValue("PruneOnFetch", ui->pruneOnFetch->isChecked());
   settings.setLocalValue("UpdateOnPull", ui->updateOnPull->isChecked());
   settings.setLocalValue("MaxCommits", ui->sbMaxCommits->value());

   delete ui;
}
