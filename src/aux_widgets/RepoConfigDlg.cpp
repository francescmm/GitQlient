#include "RepoConfigDlg.h"
#include "ui_RepoConfigDlg.h"

#include <GitConfig.h>
#include <GitQlientStyles.h>
#include <GitQlientSettings.h>
#include <GitBase.h>

#include <QLabel>
#include <QLineEdit>
#include <QStyle>
#include <QDir>
#include <QProcess>
#include <QStandardPaths>

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

RepoConfigDlg::RepoConfigDlg(const QSharedPointer<GitBase> &git, QWidget *parent)
   : QDialog(parent)
   , ui(new Ui::RepoConfigDlg)
   , mGit(git)
{
   ui->setupUi(this);

   ui->labelClangFormat->setVisible(false);
   ui->clangFormat->setVisible(false);

   GitQlientSettings settings;
   ui->autoFetch->setValue(settings.localValue(mGit->getGitQlientSettingsDir(), "AutoFetch", 5).toInt());
   ui->pruneOnFetch->setChecked(settings.localValue(mGit->getGitQlientSettingsDir(), "PruneOnFetch", true).toBool());
   ui->clangFormat->setChecked(
       settings.localValue(mGit->getGitQlientSettingsDir(), "ClangFormatOnCommit", false).toBool());
   ui->updateOnPull->setChecked(settings.localValue(mGit->getGitQlientSettingsDir(), "UpdateOnPull", false).toBool());
   ui->sbMaxCommits->setValue(settings.localValue(mGit->getGitQlientSettingsDir(), "MaxCommits", 0).toInt());

   ui->tabWidget->setCurrentIndex(0);
   connect(ui->pbClearCache, &ButtonLink::clicked, this, &RepoConfigDlg::clearCache);

   const auto isConfigured = settings.localValue(mGit->getGitQlientSettingsDir(), "BuildSystemEanbled", false).toBool();
   ui->chBoxBuildSystem->setChecked(isConfigured);
   connect(ui->chBoxBuildSystem, &QCheckBox::stateChanged, this, &RepoConfigDlg::toggleBsAccesInfo);

   ui->leBsUser->setVisible(isConfigured);
   ui->leBsUserLabel->setVisible(isConfigured);
   ui->leBsToken->setVisible(isConfigured);
   ui->leBsTokenLabel->setVisible(isConfigured);
   ui->leBsUrl->setVisible(isConfigured);
   ui->leBsUrlLabel->setVisible(isConfigured);

   if (isConfigured)
   {
      const auto url = settings.localValue(mGit->getGitQlientSettingsDir(), "BuildSystemUrl", "").toString();
      const auto user = settings.localValue(mGit->getGitQlientSettingsDir(), "BuildSystemUser", "").toString();
      const auto token = settings.localValue(mGit->getGitQlientSettingsDir(), "BuildSystemToken", "").toString();

      ui->leBsUrl->setText(url);
      ui->leBsUser->setText(user);
      ui->leBsToken->setText(token);
   }

   QScopedPointer<GitConfig> gitConfig(new GitConfig(mGit));
   const auto localConfig = gitConfig->getLocalConfig();

   if (localConfig.success)
   {
      const auto layout = new QGridLayout(ui->tab);
      layout->setContentsMargins(10, 10, 10, 10);
      layout->setSpacing(10);
      layout->addWidget(new QLabel("KEY"), 0, 0);
      layout->addWidget(new QLabel("VALUE"), 0, 1);

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
      const auto elements = localConfig.output.toString().split('\n', Qt::SkipEmptyParts);
#else
      const auto elements = localConfig.output.toString().split('\n', QString::SkipEmptyParts);
#endif
      addUserConfig(elements, layout);
   }

   const auto globalConfig = gitConfig->getGlobalConfig();

   if (globalConfig.success)
   {
      const auto layout = new QGridLayout(ui->tab_2);
      layout->setContentsMargins(10, 10, 10, 10);
      layout->setSpacing(10);
      layout->addWidget(new QLabel("KEY"), 0, 0);
      layout->addWidget(new QLabel("VALUE"), 0, 1);

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
      const auto elements = globalConfig.output.toString().split('\n', Qt::SkipEmptyParts);
#else
      const auto elements = globalConfig.output.toString().split('\n', QString::SkipEmptyParts);
#endif
      addUserConfig(elements, layout);
   }

   QString color = GitQlientStyles::getTabColor().name();

   ui->tab->setStyleSheet(QString("background-color: %1;").arg(color));
   ui->tab_2->setStyleSheet(QString("background-color: %1;").arg(color));
   ui->tab_3->setStyleSheet(QString("background-color: %1;").arg(color));
   ui->tab_4->setStyleSheet(QString("background-color: %1;").arg(color));

   style()->unpolish(this);
   setStyleSheet(GitQlientStyles::getStyles());
   style()->polish(this);

   setAttribute(Qt::WA_DeleteOnClose);

   calculateCacheSize();
}

RepoConfigDlg::~RepoConfigDlg()
{
   GitQlientSettings settings;
   settings.setLocalValue(mGit->getGitQlientSettingsDir(), "AutoFetch", ui->autoFetch->value());
   settings.setLocalValue(mGit->getGitQlientSettingsDir(), "PruneOnFetch", ui->pruneOnFetch->isChecked());
   settings.setLocalValue(mGit->getGitQlientSettingsDir(), "ClangFormatOnCommit", ui->clangFormat->isChecked());
   settings.setLocalValue(mGit->getGitQlientSettingsDir(), "UpdateOnPull", ui->updateOnPull->isChecked());
   settings.setLocalValue(mGit->getGitQlientSettingsDir(), "MaxCommits", ui->sbMaxCommits->value());

   const auto showBs = ui->chBoxBuildSystem->isChecked();
   const auto bsUser = ui->leBsUser->text();
   const auto bsToken = ui->leBsToken->text();
   const auto bsUrl = ui->leBsUrl->text();

   if (showBs && !bsUser.isEmpty() && !bsToken.isEmpty() && !bsUrl.isEmpty())
   {
      settings.setLocalValue(mGit->getGitQlientSettingsDir(), "BuildSystemEanbled", showBs);
      settings.setLocalValue(mGit->getGitQlientSettingsDir(), "BuildSystemUrl", bsUrl);
      settings.setLocalValue(mGit->getGitQlientSettingsDir(), "BuildSystemUser", bsUser);
      settings.setLocalValue(mGit->getGitQlientSettingsDir(), "BuildSystemToken", bsToken);
   }
   else
      settings.setLocalValue(mGit->getGitQlientSettingsDir(), "BuildSystemEanbled", false);

   delete ui;
}

void RepoConfigDlg::addUserConfig(const QStringList &elements, QGridLayout *layout)
{
   auto row = 1;

   for (const auto &element : elements)
   {
      if (element.startsWith("user."))
      {
         const auto pair = element.split("=");
         layout->addWidget(new QLabel(pair.first()), row, 0);

         const auto lineEdit = new QLineEdit();
         lineEdit->setText(pair.last());
         connect(lineEdit, &QLineEdit::editingFinished, this, &RepoConfigDlg::setConfig);

         layout->addWidget(lineEdit, row++, 1);

         mLineeditKeyMap.insert(lineEdit, pair.first());
      }
   }

   layout->addItem(new QSpacerItem(1, 1, QSizePolicy::Fixed, QSizePolicy::Expanding), row, 0);
}

void RepoConfigDlg::setConfig()
{
   const auto lineEdit = qobject_cast<QLineEdit *>(sender());
   QScopedPointer<GitConfig> gitConfig(new GitConfig(mGit));

   switch (ui->tabWidget->currentIndex())
   {
      case 0: // Local
         gitConfig->setLocalData(mLineeditKeyMap.value(lineEdit), lineEdit->text());
         break;
      case 1: // Global
         gitConfig->setGlobalData(mLineeditKeyMap.value(lineEdit), lineEdit->text());
         break;
      default:
         break;
   }
}

void RepoConfigDlg::clearCache()
{
   const auto path = QString("%1").arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
   QProcess p;
   p.setWorkingDirectory(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
   p.start("rm", { "-rf", path });

   if (p.waitForFinished())
      calculateCacheSize();
}

void RepoConfigDlg::calculateCacheSize()
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

void RepoConfigDlg::toggleBsAccesInfo()
{
   const auto visible = ui->chBoxBuildSystem->isChecked();
   ui->leBsUser->setVisible(visible);
   ui->leBsUserLabel->setVisible(visible);
   ui->leBsToken->setVisible(visible);
   ui->leBsTokenLabel->setVisible(visible);
   ui->leBsUrl->setVisible(visible);
   ui->leBsUrlLabel->setVisible(visible);
}
