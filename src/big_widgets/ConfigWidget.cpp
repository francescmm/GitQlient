#include "ConfigWidget.h"
#include "ui_ConfigWidget.h"

#include <GitQlientSettings.h>
#include <GitBase.h>
#include <FileEditor.h>
#include <QLogger.h>

#include <QProcess>
#include <QPushButton>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QTimer>

using namespace QLogger;

namespace
{
qint64 dirSize(QString dirPath)
{
   qint64 size = 0;
   QDir dir(dirPath);

   auto entryList = dir.entryList(QDir::Files | QDir::System | QDir::Hidden);

   for (const auto &filePath : qAsConst(entryList))
   {
      QFileInfo fi(dir, filePath);
      size += fi.size();
   }

   entryList = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::System | QDir::Hidden);

   for (const auto &childDirPath : qAsConst(entryList))
      size += dirSize(dirPath + QDir::separator() + childDirPath);

   return size;
}
}

ConfigWidget::ConfigWidget(const QSharedPointer<GitBase> &git, QWidget *parent)
   : QWidget(parent)
   , ui(new Ui::ConfigWidget)
   , mGit(git)
   , mFeedbackTimer(new QTimer())
   , mSave(new QPushButton())
{
   ui->setupUi(this);

   mFeedbackTimer->setInterval(3000);

   mSave->setIcon(QIcon(":/icons/save"));
   mSave->setToolTip(tr("Save"));
   connect(mSave, &QPushButton::clicked, this, &ConfigWidget::saveFile);
   ui->tabWidget->setCornerWidget(mSave);

   ui->mainLayout->setColumnStretch(0, 1);
   ui->mainLayout->setColumnStretch(1, 2);

   const auto localGitLayout = new QVBoxLayout(ui->localGit);
   localGitLayout->setContentsMargins(QMargins());

   const auto localGit = new FileEditor(false, this);
   localGit->editFile(mGit->getGitDir().append("/config"));
   localGitLayout->addWidget(localGit);
   mEditors.insert(0, localGit);

   const auto globalGitLayout = new QVBoxLayout(ui->globalGit);
   globalGitLayout->setContentsMargins(QMargins());

   const auto globalGit = new FileEditor(false, this);
   globalGit->editFile(
       QString("%1/%2").arg(QStandardPaths::writableLocation(QStandardPaths::HomeLocation), ".gitconfig"));
   globalGitLayout->addWidget(globalGit);
   mEditors.insert(1, globalGit);

   GitQlientSettings settings;

   ui->chDevMode->setChecked(settings.localValue(mGit->getGitDir(), "DevMode", false).toBool());
   enableWidgets();

   // GitQlient configuration
   ui->chDisableLogs->setChecked(settings.globalValue("logsDisabled", true).toBool());
   ui->cbLogLevel->setCurrentIndex(settings.globalValue("logsLevel", static_cast<int>(LogLevel::Warning)).toInt());
   ui->spCommitTitleLength->setValue(settings.globalValue("commitTitleMaxLength", 50).toInt());

   const auto currentStyle = settings.globalValue("colorSchema", "dark").toString();
   ui->cbStyle->setCurrentText(currentStyle);
   connect(ui->cbStyle, &QComboBox::currentTextChanged, this, [this, currentStyle](const QString &newText) {
      if (newText != currentStyle)
         mShowResetMsg = true;
   });

   // Repository configuration
   mOriginalRepoOrder = settings.localValue(mGit->getGitDir(), "GraphSortingOrder", 0).toInt();
   ui->cbLogOrder->setCurrentIndex(mOriginalRepoOrder);
   ui->autoFetch->setValue(settings.localValue(mGit->getGitDir(), "AutoFetch", 5).toInt());
   ui->pruneOnFetch->setChecked(settings.localValue(mGit->getGitDir(), "PruneOnFetch", true).toBool());
   ui->clangFormat->setChecked(
       settings.localValue(mGit->getGitDir(), "ClangFormatOnCommit", false).toBool());
   ui->updateOnPull->setChecked(settings.localValue(mGit->getGitDir(), "UpdateOnPull", false).toBool());
   ui->sbMaxCommits->setValue(settings.localValue(mGit->getGitDir(), "MaxCommits", 0).toInt());

   ui->tabWidget->setCurrentIndex(0);
   connect(ui->pbClearCache, &ButtonLink::clicked, this, &ConfigWidget::clearCache);

   ui->cbPomodoroEnabled->setChecked(
       settings.localValue(mGit->getGitDir(), "Pomodoro/Enabled", true).toBool());

   ui->cbStash->setChecked(settings.localValue(mGit->getGitDir(), "StashesHeader", true).toBool());
   ui->cbSubmodule->setChecked(settings.localValue(mGit->getGitDir(), "SubmodulesHeader", true).toBool());
   ui->cbSubtree->setChecked(settings.localValue(mGit->getGitDir(), "SubtreeHeader", true).toBool());

   // Build System configuration
   const auto isConfigured = settings.localValue(mGit->getGitDir(), "BuildSystemEanbled", false).toBool();
   ui->chBoxBuildSystem->setChecked(isConfigured);
   connect(ui->chBoxBuildSystem, &QCheckBox::stateChanged, this, &ConfigWidget::toggleBsAccesInfo);

   ui->leBsUser->setVisible(isConfigured);
   ui->leBsUserLabel->setVisible(isConfigured);
   ui->leBsToken->setVisible(isConfigured);
   ui->leBsTokenLabel->setVisible(isConfigured);
   ui->leBsUrl->setVisible(isConfigured);
   ui->leBsUrlLabel->setVisible(isConfigured);

   if (isConfigured)
   {
      const auto url = settings.localValue(mGit->getGitDir(), "BuildSystemUrl", "").toString();
      const auto user = settings.localValue(mGit->getGitDir(), "BuildSystemUser", "").toString();
      const auto token = settings.localValue(mGit->getGitDir(), "BuildSystemToken", "").toString();

      ui->leBsUrl->setText(url);
      ui->leBsUser->setText(user);
      ui->leBsToken->setText(token);
   }

   // Connects for automatic save
   connect(ui->chDevMode, &CheckBox::stateChanged, this, &ConfigWidget::enableWidgets);
   connect(ui->chDisableLogs, &CheckBox::stateChanged, this, &ConfigWidget::saveConfig);
   connect(ui->cbLogLevel, SIGNAL(currentIndexChanged(int)), this, SLOT(saveConfig()));
   connect(ui->cbStyle, SIGNAL(currentIndexChanged(int)), this, SLOT(saveConfig()));
   connect(ui->leGitPath, &QLineEdit::editingFinished, this, &ConfigWidget::saveConfig);
   connect(ui->spCommitTitleLength, SIGNAL(valueChanged(int)), this, SLOT(saveConfig()));
   connect(ui->cbTranslations, SIGNAL(currentIndexChanged(int)), this, SLOT(saveConfig()));
   connect(ui->sbMaxCommits, SIGNAL(valueChanged(int)), this, SLOT(saveConfig()));
   connect(ui->cbLogOrder, SIGNAL(currentIndexChanged(int)), this, SLOT(saveConfig()));
   connect(ui->autoFetch, SIGNAL(valueChanged(int)), this, SLOT(saveConfig()));
   connect(ui->pruneOnFetch, &QCheckBox::stateChanged, this, &ConfigWidget::saveConfig);
   connect(ui->updateOnPull, &QCheckBox::stateChanged, this, &ConfigWidget::saveConfig);
   connect(ui->clangFormat, &QCheckBox::stateChanged, this, &ConfigWidget::saveConfig);
   connect(ui->cbPomodoroEnabled, &QCheckBox::stateChanged, this, &ConfigWidget::saveConfig);
   connect(ui->cbStash, &QCheckBox::stateChanged, this, &ConfigWidget::saveConfig);
   connect(ui->cbSubmodule, &QCheckBox::stateChanged, this, &ConfigWidget::saveConfig);
   connect(ui->cbSubtree, &QCheckBox::stateChanged, this, &ConfigWidget::saveConfig);
   connect(ui->leBsUrl, &QLineEdit::editingFinished, this, &ConfigWidget::saveConfig);
   connect(ui->leBsUser, &QLineEdit::editingFinished, this, &ConfigWidget::saveConfig);
   connect(ui->leBsToken, &QLineEdit::editingFinished, this, &ConfigWidget::saveConfig);
}

ConfigWidget::~ConfigWidget()
{
   delete ui;
}

void ConfigWidget::onPanelsVisibilityChanged()
{
   GitQlientSettings settings;

   ui->cbStash->setChecked(settings.localValue(mGit->getGitDir(), "StashesHeader", true).toBool());
   ui->cbSubmodule->setChecked(settings.localValue(mGit->getGitDir(), "SubmodulesHeader", true).toBool());
   ui->cbSubtree->setChecked(settings.localValue(mGit->getGitDir(), "SubtreeHeader", true).toBool());
}

void ConfigWidget::clearCache()
{
   const auto path = QString("%1").arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
   QProcess p;
   p.setWorkingDirectory(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
   p.start("rm", { "-rf", path });

   if (p.waitForFinished())
      calculateCacheSize();
}

void ConfigWidget::calculateCacheSize()
{
   auto size = 0;
   const auto dirPath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
   QDir dir(dirPath);
   QDir::Filters dirFilters = QDir::Dirs | QDir::NoDotAndDotDot | QDir::System | QDir::Hidden | QDir::Files;
   const auto &list = dir.entryInfoList(dirFilters);

   for (const QFileInfo &file : list)
   {
      size += file.size();
      size += dirSize(dirPath + "/" + file.fileName());
   }

   ui->lCacheSize->setText(QString("%1 KB").arg(size / 1024.0));
}

void ConfigWidget::toggleBsAccesInfo()
{
   const auto visible = ui->chBoxBuildSystem->isChecked();
   ui->leBsUser->setVisible(visible);
   ui->leBsUserLabel->setVisible(visible);
   ui->leBsToken->setVisible(visible);
   ui->leBsTokenLabel->setVisible(visible);
   ui->leBsUrl->setVisible(visible);
   ui->leBsUrlLabel->setVisible(visible);
}

void ConfigWidget::saveConfig()
{
   mFeedbackTimer->stop();

   ui->lFeedback->setText(tr("Changes saved"));

   GitQlientSettings settings;

   settings.setGlobalValue("logsDisabled", ui->chDisableLogs->isChecked());
   settings.setGlobalValue("logsLevel", ui->cbLogLevel->currentIndex());
   settings.setGlobalValue("commitTitleMaxLength", ui->spCommitTitleLength->value());
   settings.setGlobalValue("colorSchema", ui->cbStyle->currentText());
   settings.setGlobalValue("gitLocation", ui->leGitPath->text());

   emit commitTitleMaxLenghtChanged();

   if (mShowResetMsg)
   {
      QMessageBox::information(this, tr("Reset needed!"),
                               tr("You need to restart GitQlient to see the changes in the styles applid."));
   }

   const auto logger = QLoggerManager::getInstance();
   logger->overwriteLogLevel(static_cast<LogLevel>(ui->cbLogLevel->currentIndex()));

   if (ui->chDisableLogs->isChecked())
      logger->pause();
   else
      logger->resume();

   if (mOriginalRepoOrder != ui->cbLogOrder->currentIndex())
   {
      settings.setLocalValue(mGit->getGitDir(), "GraphSortingOrder", ui->cbLogOrder->currentIndex());
      emit reloadView();
   }

   settings.setLocalValue(mGit->getGitDir(), "AutoFetch", ui->autoFetch->value());
   settings.setLocalValue(mGit->getGitDir(), "PruneOnFetch", ui->pruneOnFetch->isChecked());
   settings.setLocalValue(mGit->getGitDir(), "ClangFormatOnCommit", ui->clangFormat->isChecked());
   settings.setLocalValue(mGit->getGitDir(), "UpdateOnPull", ui->updateOnPull->isChecked());
   settings.setLocalValue(mGit->getGitDir(), "MaxCommits", ui->sbMaxCommits->value());

   settings.setLocalValue(mGit->getGitDir(), "StashesHeader", ui->cbStash->isChecked());
   settings.setLocalValue(mGit->getGitDir(), "SubmodulesHeader", ui->cbSubmodule->isChecked());
   settings.setLocalValue(mGit->getGitDir(), "SubtreeHeader", ui->cbSubtree->isChecked());

   emit panelsVisibilityChaned();

   /* POMODORO CONFIG */
   settings.setLocalValue(mGit->getGitDir(), "Pomodoro/Enabled", ui->cbPomodoroEnabled->isChecked());

   /* BUILD SYSTEM CONFIG */

   const auto showBs = ui->chBoxBuildSystem->isChecked();
   const auto bsUser = ui->leBsUser->text();
   const auto bsToken = ui->leBsToken->text();
   const auto bsUrl = ui->leBsUrl->text();

   if (showBs && !bsUser.isEmpty() && !bsToken.isEmpty() && !bsUrl.isEmpty())
   {
      settings.setLocalValue(mGit->getGitDir(), "BuildSystemEanbled", showBs);
      settings.setLocalValue(mGit->getGitDir(), "BuildSystemUrl", bsUrl);
      settings.setLocalValue(mGit->getGitDir(), "BuildSystemUser", bsUser);
      settings.setLocalValue(mGit->getGitDir(), "BuildSystemToken", bsToken);
      emit buildSystemConfigured(showBs);
   }
   else
   {
      settings.setLocalValue(mGit->getGitDir(), "BuildSystemEanbled", false);
      emit buildSystemConfigured(false);
   }

   mFeedbackTimer->singleShot(3000, ui->lFeedback, &QLabel::clear);
}

void ConfigWidget::enableWidgets()
{
   const auto enable = ui->chDevMode->isChecked();

   GitQlientSettings settings;
   settings.setLocalValue(mGit->getGitDir(), "DevMode", enable);

   ui->tabWidget->setEnabled(enable);
}

void ConfigWidget::saveFile()
{
   const auto id = ui->tabWidget->currentIndex();
   mEditors.value(id)->saveFile();
}
