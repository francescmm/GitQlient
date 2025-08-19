#include "Controls.h"

#include <BranchDlg.h>
#include <ConfigWidget.h>
#include <GitBase.h>
#include <GitCache.h>
#include <GitConfig.h>
#include <GitQlientSettings.h>
#include <GitQlientStyles.h>
#include <GitQlientUpdater.h>
#include <GitRemote.h>
#include <GitStashes.h>
#include <PomodoroButton.h>
#include <QLogger.h>

#include <QApplication>
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QMenu>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QTimer>
#include <QToolButton>
#include <qwidget.h>

using namespace QLogger;

Controls::Controls(const QSharedPointer<GitCache> &cache, const QSharedPointer<GitBase> &git, QWidget *parent)
   : QFrame(parent)
   , mCache(cache)
   , mGit(git)
   , mStashPop(createToolButton(":/icons/git_pop", tr("Stash Pop"), Qt::CTRL | Qt::Key_2))
   , mStashPush(createToolButton(":/icons/git_stash", tr("Stash Push"), Qt::CTRL | Qt::Key_3))
   , mPullBtn(createToolButton(":/icons/git_pull", tr("Pull"), Qt::CTRL | Qt::Key_4))
   , mPullOptions(createToolButton(":/icons/arrow_down", tr("Remote actions")))
   , mPushBtn(createToolButton(":/icons/git_push", tr("Push"), Qt::CTRL | Qt::Key_5))
   , mRefreshBtn(createToolButton(":/icons/refresh", tr("Refresh"), Qt::Key_F5))
   , mConfigBtn(createToolButton(":/icons/config", tr("Config"), Qt::CTRL | Qt::Key_6))
   , mVersionCheck(new QToolButton(this))
   , mMergeWarning(
         new QPushButton(tr("WARNING: There is a merge pending to be committed! Click here to solve it."), this))
   , mUpdater(new GitQlientUpdater(this))
   , mLastSeparator(new QFrame(this))
{
   GitQlientSettings settings(mGit->getGitDir());

   setAttribute(Qt::WA_DeleteOnClose);

   connect(mUpdater, &GitQlientUpdater::newVersionAvailable, this, [this]() {
      mVersionCheck->setVisible(true);
      mLastSeparator->setVisible(mVersionCheck->isVisible());
   });

   const auto menu = new QMenu(mPullOptions);
   menu->installEventFilter(this);

   auto action = menu->addAction(tr("Fetch all"));
   connect(action, &QAction::triggered, this, &Controls::fetchAll);

   action = menu->addAction(tr("Prune"));
   connect(action, &QAction::triggered, this, &Controls::pruneBranches);
   menu->addSeparator();

   mPullBtn->setPopupMode(QToolButton::InstantPopup);
   mPullBtn->setObjectName("ToolButtonAboveMenu");

   mPullOptions->setPopupMode(QToolButton::InstantPopup);
   mPullOptions->setObjectName("ToolButtonWithMenu");

   const auto pullLayout = new QVBoxLayout();
   pullLayout->setContentsMargins(QMargins());
   pullLayout->setSpacing(0);
   pullLayout->addWidget(mPullBtn);
   pullLayout->addWidget(mPullOptions);

   const auto separator = new QFrame(this);
   separator->setObjectName("orangeSeparator");
   separator->setFixedHeight(20);

   const auto separator2 = new QFrame(this);
   separator2->setObjectName("orangeSeparator");
   separator2->setFixedHeight(20);

   const auto hLayout = new QHBoxLayout();
   hLayout->setContentsMargins(QMargins());
   hLayout->addStretch();
   hLayout->setSpacing(5);
   hLayout->addWidget(mStashPush);
   hLayout->addWidget(mStashPop);
   hLayout->addWidget(separator);
   hLayout->addLayout(pullLayout);
   hLayout->addWidget(mPushBtn);
   hLayout->addWidget(separator2);

   mVersionCheck->setIcon(QIcon(":/icons/get_gitqlient"));
   mVersionCheck->setIconSize(QSize(22, 22));
   mVersionCheck->setText(tr("New version"));
   mVersionCheck->setObjectName("longToolButton");
   mVersionCheck->setToolButtonStyle(Qt::ToolButtonIconOnly);
   mVersionCheck->setVisible(false);

   mUpdater->checkNewGitQlientVersion();

   hLayout->addWidget(mRefreshBtn);
   hLayout->addWidget(mConfigBtn);

   mGitPlatform->setVisible(false);
   mBuildSystem->setVisible(false);

   mPluginsSeparator = new QFrame(this);
   mPluginsSeparator->setObjectName("orangeSeparator");
   mPluginsSeparator->setFixedHeight(20);
   mPluginsSeparator->setVisible(mBuildSystem->isVisible() || mGitPlatform->isVisible());
   hLayout->addWidget(mPluginsSeparator);

   createGitPlatformButton(hLayout);

   mBuildSystem->setCheckable(true);
   mBuildSystem->setIcon(QIcon(":/icons/build_system"));
   mBuildSystem->setIconSize(QSize(22, 22));
   mBuildSystem->setToolTip("Jenkins");
   mBuildSystem->setToolButtonStyle(Qt::ToolButtonIconOnly);
   mBuildSystem->setPopupMode(QToolButton::InstantPopup);
   mBuildSystem->setShortcut(Qt::CTRL | Qt::Key_9);
   mBtnGroup->addButton(mBuildSystem, static_cast<int>(ControlsMainViews::BuildSystem));

   hLayout->addWidget(mBuildSystem);

   configBuildSystemButton();

   mBuildSystem->setEnabled(settings.localValue("BuildSystemEnabled", false).toBool());
   mGitPlatform->setEnabled(settings.localValue("GitServerEnabled", false).toBool());

   mLastSeparator->setObjectName("orangeSeparator");
   mLastSeparator->setFixedHeight(20);
   mLastSeparator->setVisible(mVersionCheck->isVisible());

   hLayout->addWidget(mLastSeparator);
   hLayout->addWidget(mVersionCheck);
   hLayout->addStretch();

   mMergeWarning->setObjectName("WarningButton");
   mMergeWarning->setVisible(false);

   const auto vLayout = new QVBoxLayout(this);
   vLayout->setContentsMargins(0, 5, 0, 0);
   vLayout->setSpacing(10);
   vLayout->addLayout(hLayout);
   vLayout->addWidget(mMergeWarning);

   connect(mStashPush, &QToolButton::clicked, this, &Controls::stashPush);
   connect(mStashPop, &QToolButton::clicked, this, &Controls::stashPop);
   connect(mPullBtn, &QToolButton::clicked, this, &Controls::pullCurrentBranch);
   connect(mPushBtn, &QToolButton::clicked, this, &Controls::pushCurrentBranch);
   connect(mRefreshBtn, &QToolButton::clicked, this, &Controls::requestFullReload);
   connect(mMergeWarning, &QPushButton::clicked, this, &Controls::signalGoMerge);
   connect(mVersionCheck, &QToolButton::clicked, mUpdater, &GitQlientUpdater::showInfoMessage);
   connect(mConfigBtn, &QToolButton::clicked, this, &Controls::showConfigDialog);

   enableButtons(false);
}

void Controls::enableButtons(bool enabled)
{
   mStashPush->setEnabled(enabled);
   mPullBtn->setEnabled(enabled);
   mPullOptions->setEnabled(enabled);
   mPushBtn->setEnabled(enabled);
   mRefreshBtn->setEnabled(enabled);
   mConfigBtn->setEnabled(enabled);

   if (enabled)
   {
      GitQlientSettings settings(mGit->getGitDir());

      mBuildSystem->setEnabled(settings.localValue("BuildSystemEnabled", false).toBool());
      mGitPlatform->setEnabled(settings.localValue("GitServerEnabled", false).toBool());
   }
   else
      mBuildSystem->setEnabled(false);
}

void Controls::stashPush()
{
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   QScopedPointer<GitStashes> git(new GitStashes(mGit));
   const auto ret = git->stash();
   QApplication::restoreOverrideCursor();

   if (ret.success)
   {
      QLog_Debug("Controls", "Stash push successful");
      emit requestFullReload();
   }
   else
   {
      QMessageBox msgBox(QMessageBox::Critical, tr("Error while stashing"),
                         QString(tr("There were problems during the stash operation. Please, see the detailed "
                                    "description for more information.")),
                         QMessageBox::Ok, this);
      msgBox.setDetailedText(ret.output);
      msgBox.setStyleSheet(GitQlientStyles::getStyles());
      msgBox.exec();
   }
}

void Controls::stashPop()
{
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   QScopedPointer<GitStashes> git(new GitStashes(mGit));
   const auto ret = git->pop();
   QApplication::restoreOverrideCursor();

   if (ret.success)
   {
      QLog_Debug("Controls", "Stash pop successful");

      if (ret.output.contains("CONFLICT", Qt::CaseInsensitive))
      {
         QMessageBox::warning(this, tr("Stash pop conflicts"),
                              tr("There were merge conflicts when applying the stash. "
                                 "Please resolve them manually."),
                              QMessageBox::Ok);
         emit signalPullConflict();
      }

      emit requestFullReload();
   }
   else
   {
      if (ret.output.contains("No stash entries found", Qt::CaseInsensitive) || ret.output.contains("No stash found", Qt::CaseInsensitive))
      {
         QMessageBox::information(this, tr("No stashes"),
                                  tr("There are no stashes to pop."),
                                  QMessageBox::Ok);
      }
      else
      {
         QMessageBox msgBox(QMessageBox::Critical, tr("Error while popping stash"),
                            QString(tr("There were problems during the stash pop operation. Please, see the detailed "
                                       "description for more information.")),
                            QMessageBox::Ok, this);
         msgBox.setDetailedText(ret.output);
         msgBox.setStyleSheet(GitQlientStyles::getStyles());
         msgBox.exec();
      }
   }
}

void Controls::pullCurrentBranch()
{
   GitQlientSettings settings(mGit->getGitDir());
   const auto updateOnPull = settings.localValue("UpdateOnPull", true).toBool();

   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   QScopedPointer<GitRemote> git(new GitRemote(mGit));
   const auto ret = git->pull(updateOnPull);
   QApplication::restoreOverrideCursor();

   if (ret.success)
   {
      if (ret.output.contains("merge conflict", Qt::CaseInsensitive))
         emit signalPullConflict();
      else
         emit requestFullReload();
   }
   else
   {
      if (ret.output.contains("error: could not apply", Qt::CaseInsensitive)
          && ret.output.contains("causing a conflict", Qt::CaseInsensitive))
      {
         emit signalPullConflict();
      }
      else
      {
         QMessageBox msgBox(QMessageBox::Critical, tr("Error while pulling"),
                            QString(tr("There were problems during the pull operation. Please, see the detailed "
                                       "description for more information.")),
                            QMessageBox::Ok, this);
         msgBox.setDetailedText(ret.output);
         msgBox.setStyleSheet(GitQlientStyles::getStyles());
         msgBox.exec();
      }
   }
}

void Controls::fetchAll()
{
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   GitQlientSettings settings(mGit->getGitDir());
   QScopedPointer<GitRemote> git(new GitRemote(mGit));
   const auto ret = git->fetch(settings.localValue("PruneOnFetch").toBool());
   QApplication::restoreOverrideCursor();

   if (!ret)
      emit requestFullReload();
}

void Controls::activateMergeWarning()
{
   mMergeWarning->setVisible(true);
}

void Controls::disableMergeWarning()
{
   mMergeWarning->setVisible(false);
}

void Controls::disableDiff()
{
   mStashPop->setDisabled(true);
}

void Controls::enableDiff()
{
   mStashPop->setEnabled(true);
}

ControlsMainViews Controls::getCurrentSelectedButton() const
{
   return mStashPush->isChecked() ? ControlsMainViews::Blame : ControlsMainViews::History;
}

QToolButton *Controls::createToolButton(const QString &iconPath, const QString &tooltip,
                                        const QKeySequence &shortcut)
{
   auto button = new QToolButton(this);
   button->setIcon(QIcon(iconPath));
   button->setIconSize(QSize(22, 22));
   button->setToolTip(tooltip);
   button->setToolButtonStyle(Qt::ToolButtonIconOnly);
   if (!shortcut.isEmpty())
   {
      button->setShortcut(shortcut);
   }
   return button;
}

void Controls::pushCurrentBranch()
{
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   QScopedPointer<GitRemote> git(new GitRemote(mGit));
   const auto ret = git->push();
   QApplication::restoreOverrideCursor();

   if (ret.output.contains("has no upstream branch"))
   {
      const auto currentBranch = mGit->getCurrentBranch();
      BranchDlg dlg({ currentBranch, BranchDlgMode::PUSH_UPSTREAM, mCache, mGit });
      const auto dlgRet = dlg.exec();

      if (dlgRet == QDialog::Accepted)
         emit signalRefreshPRsCache();
   }
   else if (ret.success)
   {
      const auto currentBranch = mGit->getCurrentBranch();
      QScopedPointer<GitConfig> git(new GitConfig(mGit));
      const auto remote = git->getRemoteForBranch(currentBranch);

      if (remote.success)
      {
         const auto oldSha = mCache->getShaOfReference(QString("%1/%2").arg(remote.output, currentBranch),
                                                       References::Type::RemoteBranches);
         const auto sha = mCache->getShaOfReference(currentBranch, References::Type::LocalBranch);
         mCache->deleteReference(oldSha, References::Type::RemoteBranches,
                                 QString("%1/%2").arg(remote.output, currentBranch));
         mCache->insertReference(sha, References::Type::RemoteBranches,
                                 QString("%1/%2").arg(remote.output, currentBranch));
         emit mCache->signalCacheUpdated();
         emit signalRefreshPRsCache();
      }
   }
   else
   {
      QMessageBox msgBox(
          QMessageBox::Critical, tr("Error while pushing"),
          QString(tr("There were problems during the push operation. Please, see the detailed description "
                     "for more information.")),
          QMessageBox::Ok, this);
      msgBox.setDetailedText(ret.output);
      msgBox.setStyleSheet(GitQlientStyles::getStyles());
      msgBox.exec();
   }
}

void Controls::pruneBranches()
{
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   QScopedPointer<GitRemote> git(new GitRemote(mGit));
   const auto ret = git->prune();
   QApplication::restoreOverrideCursor();

   if (!ret.success)
      emit requestReferencesReload();
}

void Controls::createGitPlatformButton(QHBoxLayout *layout)
{
   QScopedPointer<GitConfig> gitConfig(new GitConfig(mGit));
   const auto remoteUrl = gitConfig->getServerHost();
   QIcon gitPlatformIcon;
   QString name;
   QString prName;
   auto add = false;

   if (remoteUrl.contains("github", Qt::CaseInsensitive))
   {
      add = true;

      gitPlatformIcon = QIcon(":/icons/github");
      name = "GitHub";
      prName = tr("Pull Request");
   }
   else if (remoteUrl.contains("gitlab", Qt::CaseInsensitive))
   {
      add = true;

      gitPlatformIcon = QIcon(":/icons/gitlab");
      name = "GitLab";
      prName = tr("Merge Request");
   }

   if (add)
   {
      mGitPlatform->setCheckable(true);
      mGitPlatform->setIcon(gitPlatformIcon);
      mGitPlatform->setIconSize(QSize(22, 22));
      mGitPlatform->setToolTip(name);
      mGitPlatform->setToolButtonStyle(Qt::ToolButtonIconOnly);
      mGitPlatform->setPopupMode(QToolButton::InstantPopup);
      mGitPlatform->setShortcut(Qt::CTRL | Qt::Key_8);
      mBtnGroup->addButton(mGitPlatform, static_cast<int>(ControlsMainViews::GitServer));

      layout->addWidget(mGitPlatform);

      connect(mGitPlatform, &QToolButton::clicked, this, &Controls::signalGoServer);
   }
}

void Controls::configBuildSystemButton()
{
   GitQlientSettings settings(mGit->getGitDir());
   const auto isConfigured = settings.localValue("BuildSystemEnabled", false).toBool();
   mBuildSystem->setEnabled(isConfigured);

   if (!isConfigured)
      emit signalGoRepo();
}

bool Controls::eventFilter(QObject *obj, QEvent *event)
{
   if (const auto menu = qobject_cast<QMenu *>(obj); menu && event->type() == QEvent::Show)
   {
      auto localPos = menu->parentWidget()->pos();
      localPos.setX(localPos.x());
      auto pos = mapToGlobal(localPos);
      menu->show();
      pos.setY(pos.y() + menu->parentWidget()->height());
      menu->move(pos);
      return true;
   }

   return false;
}

void Controls::showConfigDialog()
{
   if (mConfigDialog && mConfigDialog->isVisible())
   {
      mConfigDialog->raise();
      mConfigDialog->activateWindow();
      return;
   }

   mConfigDialog = new QDialog(this);
   mConfigDialog->setWindowTitle(tr("Repository Configuration"));
   mConfigDialog->setAttribute(Qt::WA_DeleteOnClose);
   mConfigDialog->setModal(false);
   mConfigDialog->resize(800, 600);

   auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, mConfigDialog);
   buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Accept"));
   connect(buttonBox, &QDialogButtonBox::accepted, mConfigDialog, &QDialog::accept);

   auto configWidget = new ConfigWidget(mGit, mConfigDialog);

   auto layout = new QVBoxLayout(mConfigDialog);
   layout->setContentsMargins(QMargins(10, 10, 10, 10));
   layout->addWidget(configWidget);
   layout->addWidget(buttonBox);

   connect(configWidget, &ConfigWidget::reloadDiffFont, this, &Controls::requestFullReload);
   connect(configWidget, &ConfigWidget::autoFetchChanged, this, &Controls::autoFetchIntervalChanged);
   connect(configWidget, &ConfigWidget::autoRefreshChanged, this, &Controls::autoRefreshIntervalChanged);
   connect(configWidget, &ConfigWidget::autoRefreshChanged, this, &Controls::commitTitleMaxLenghtChanged);
   connect(configWidget, &ConfigWidget::panelsVisibilityChanged, this, &Controls::panelsVisibilityChanged);
   connect(configWidget, &ConfigWidget::moveLogsAndClose, this, [this]() {
      emit moveLogsAndClose();
      if (mConfigDialog)
         mConfigDialog->close();
   });

   mConfigDialog->show();
}
