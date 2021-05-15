#include "InitialRepoConfig.h"
#include "ui_InitialRepoConfig.h"

#include <GitBase.h>
#include <GitConfig.h>
#include <GitQlientSettings.h>
#include <GitQlientStyles.h>

InitialRepoConfig::InitialRepoConfig(const QSharedPointer<GitBase> &git,
                                     const QSharedPointer<GitQlientSettings> &settings, QWidget *parent)
   : QDialog(parent)
   , ui(new Ui::InitialRepoConfig)
   , mSettings(settings)
{
   setAttribute(Qt::WA_DeleteOnClose);

   ui->setupUi(this);

   setStyleSheet(GitQlientStyles::getInstance()->getStyles());

   ui->autoFetch->setValue(mSettings->localValue("AutoFetch", 5).toInt());
   ui->pruneOnFetch->setChecked(settings->localValue("PruneOnFetch", true).toBool());
   ui->updateOnPull->setChecked(settings->localValue("UpdateOnPull", false).toBool());
   ui->sbMaxCommits->setValue(settings->localValue("MaxCommits", 0).toInt());

   QScopedPointer<GitConfig> gitConfig(new GitConfig(git));

   const auto url = gitConfig->getServerUrl();
   ui->credentialsFrames->setVisible(url.startsWith("https"));
}

InitialRepoConfig::~InitialRepoConfig()
{
   mSettings->setLocalValue("AutoFetch", ui->autoFetch->value());
   mSettings->setLocalValue("PruneOnFetch", ui->pruneOnFetch->isChecked());
   mSettings->setLocalValue("UpdateOnPull", ui->updateOnPull->isChecked());
   mSettings->setLocalValue("MaxCommits", ui->sbMaxCommits->value());

   delete ui;
}
