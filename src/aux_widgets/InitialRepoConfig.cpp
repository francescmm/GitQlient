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

   ui->autoFetch->setValue(settings.localValue(mGit->getGitQlientSettingsDir(), "AutoFetch", 5).toInt());
   ui->pruneOnFetch->setChecked(settings.localValue(mGit->getGitQlientSettingsDir(), "PruneOnFetch", true).toBool());
   ui->updateOnPull->setChecked(settings.localValue(mGit->getGitQlientSettingsDir(), "UpdateOnPull", false).toBool());
   ui->sbMaxCommits->setValue(settings.localValue(mGit->getGitQlientSettingsDir(), "MaxCommits", 0).toInt());
}

InitialRepoConfig::~InitialRepoConfig()
{
   GitQlientSettings settings;
   settings.setLocalValue(mGit->getGitQlientSettingsDir(), "AutoFetch", ui->autoFetch->value());
   settings.setLocalValue(mGit->getGitQlientSettingsDir(), "PruneOnFetch", ui->pruneOnFetch->isChecked());
   settings.setLocalValue(mGit->getGitQlientSettingsDir(), "UpdateOnPull", ui->updateOnPull->isChecked());
   settings.setLocalValue(mGit->getGitQlientSettingsDir(), "MaxCommits", ui->sbMaxCommits->value());

   delete ui;
}
