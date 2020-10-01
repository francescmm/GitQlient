#include "GitQlientRepo.h"

#include <GitQlientSettings.h>
#include <GitTags.h>
#include <Controls.h>
#include <BranchesWidget.h>
#include <CommitHistoryColumns.h>
#include <HistoryWidget.h>
#include <QLogger.h>
#include <BlameWidget.h>
#include <CommitInfo.h>
#include <WaitingDlg.h>
#include <GitConfigDlg.h>
#include <Controls.h>
#include <HistoryWidget.h>
#include <DiffWidget.h>
#include <MergeWidget.h>
#include <GitCache.h>
#include <GitRepoLoader.h>
#include <GitConfig.h>
#include <GitBase.h>
#include <GitHistory.h>
#include <GitHubRestApi.h>
#include <GitServerWidget.h>
#include <GitServerCache.h>
#include <ConfigData.h>
#include <JenkinsWidget.h>

#include <QTimer>
#include <QDirIterator>
#include <QFileSystemWatcher>
#include <QFileDialog>
#include <QMessageBox>
#include <QStackedWidget>
#include <QGridLayout>
#include <QApplication>
#include <QStackedLayout>

using namespace QLogger;
using namespace GitServer;
using namespace Jenkins;

GitQlientRepo::GitQlientRepo(const QString &repoPath, QWidget *parent)
   : QFrame(parent)
   , mGitQlientCache(new GitCache())
   , mGitServerCache(new GitServerCache())
   , mGitBase(new GitBase(repoPath))
   , mGitLoader(new GitRepoLoader(mGitBase, mGitQlientCache))
   , mHistoryWidget(new HistoryWidget(mGitQlientCache, mGitBase, mGitServerCache))
   , mStackedLayout(new QStackedLayout())
   , mControls(new Controls(mGitQlientCache, mGitBase))
   , mDiffWidget(new DiffWidget(mGitBase, mGitQlientCache))
   , mBlameWidget(new BlameWidget(mGitQlientCache, mGitBase))
   , mMergeWidget(new MergeWidget(mGitQlientCache, mGitBase))
   , mGitServerWidget(new GitServerWidget(mGitQlientCache, mGitBase, mGitServerCache))
   , mJenkins(new JenkinsWidget(mGitBase))
   , mAutoFetch(new QTimer())
   , mAutoFilesUpdate(new QTimer())
{
   setAttribute(Qt::WA_DeleteOnClose);

   QLog_Info("UI", QString("Initializing GitQlient"));

   setObjectName("mainWindow");
   setWindowTitle("GitQlient");

   QScopedPointer<GitConfig> gitConfig(new GitConfig(mGitBase));
   const auto serverUrl = gitConfig->getServerUrl();
   const auto repoInfo = gitConfig->getCurrentRepoAndOwner();

   mGitServerCache->init(serverUrl, repoInfo);

   mStackedLayout->addWidget(mHistoryWidget);
   mStackedLayout->addWidget(mDiffWidget);
   mStackedLayout->addWidget(mBlameWidget);
   mStackedLayout->addWidget(mMergeWidget);
   mStackedLayout->addWidget(mGitServerWidget);
   mStackedLayout->addWidget(mJenkins);

   const auto mainLayout = new QVBoxLayout(this);
   mainLayout->setSpacing(0);
   mainLayout->setContentsMargins(10, 0, 0, 10);
   mainLayout->addWidget(mControls);
   mainLayout->addLayout(mStackedLayout);

   showHistoryView();

   GitQlientSettings settings;
   const auto fetchInterval = settings.localValue(mGitBase->getGitQlientSettingsDir(), "AutoFetch", 5).toInt();

   mAutoFetch->setInterval(fetchInterval * 60 * 1000);
   mAutoFilesUpdate->setInterval(15000);

   connect(mAutoFetch, &QTimer::timeout, mControls, &Controls::fetchAll);
   connect(mAutoFilesUpdate, &QTimer::timeout, this, &GitQlientRepo::updateUiFromWatcher);

   connect(mControls, &Controls::signalFetchPerformed, this, &GitQlientRepo::updateTagsOnCache);
   connect(mControls, &Controls::signalGoRepo, this, &GitQlientRepo::showHistoryView);
   connect(mControls, &Controls::signalGoBlame, this, &GitQlientRepo::showBlameView);
   connect(mControls, &Controls::signalGoDiff, this, &GitQlientRepo::showDiffView);
   connect(mControls, &Controls::signalGoMerge, this, &GitQlientRepo::showMergeView);
   connect(mControls, &Controls::signalGoServer, this, &GitQlientRepo::showGitServerView);
   connect(mControls, &Controls::signalGoBuildSystem, this, &GitQlientRepo::showBuildSystemView);
   connect(mControls, &Controls::signalRepositoryUpdated, this, &GitQlientRepo::updateCache);
   connect(mControls, &Controls::signalPullConflict, mControls, &Controls::activateMergeWarning);
   connect(mControls, &Controls::signalPullConflict, this, &GitQlientRepo::showWarningMerge);

   connect(mHistoryWidget, &HistoryWidget::signalEditFile, this, &GitQlientRepo::signalEditFile);
   connect(mHistoryWidget, &HistoryWidget::signalAllBranchesActive, mGitLoader.data(), &GitRepoLoader::setShowAll);
   connect(mHistoryWidget, &HistoryWidget::signalAllBranchesActive, this, &GitQlientRepo::updateCache);
   connect(mHistoryWidget, &HistoryWidget::signalUpdateCache, this, &GitQlientRepo::updateCache);
   connect(mHistoryWidget, &HistoryWidget::signalOpenSubmodule, this, &GitQlientRepo::signalOpenSubmodule);
   connect(mHistoryWidget, &HistoryWidget::signalViewUpdated, this, &GitQlientRepo::updateCache);
   connect(mHistoryWidget, &HistoryWidget::signalOpenDiff, this, &GitQlientRepo::openCommitDiff);
   connect(mHistoryWidget, &HistoryWidget::signalOpenCompareDiff, this, &GitQlientRepo::openCommitCompareDiff);
   connect(mHistoryWidget, &HistoryWidget::signalShowDiff, this, &GitQlientRepo::loadFileDiff);
   connect(mHistoryWidget, &HistoryWidget::signalChangesCommitted, this, &GitQlientRepo::changesCommitted);
   connect(mHistoryWidget, &HistoryWidget::signalUpdateUi, this, &GitQlientRepo::updateUiFromWatcher);
   connect(mHistoryWidget, &HistoryWidget::signalShowFileHistory, this, &GitQlientRepo::showFileHistory);
   connect(mHistoryWidget, &HistoryWidget::signalMergeConflicts, mControls, &Controls::activateMergeWarning);
   connect(mHistoryWidget, &HistoryWidget::signalMergeConflicts, this, &GitQlientRepo::showWarningMerge);
   connect(mHistoryWidget, &HistoryWidget::signalCherryPickConflict, mControls, &Controls::activateMergeWarning);
   connect(mHistoryWidget, &HistoryWidget::signalCherryPickConflict, this, &GitQlientRepo::showCherryPickConflict);
   connect(mHistoryWidget, &HistoryWidget::signalPullConflict, mControls, &Controls::activateMergeWarning);
   connect(mHistoryWidget, &HistoryWidget::signalPullConflict, this, &GitQlientRepo::showWarningMerge);
   connect(mHistoryWidget, &HistoryWidget::signalUpdateWip, this, &GitQlientRepo::updateWip);
   connect(mHistoryWidget, &HistoryWidget::showPrDetailedView, this, &GitQlientRepo::showGitServerPrView);

   connect(mDiffWidget, &DiffWidget::signalShowFileHistory, this, &GitQlientRepo::showFileHistory);
   connect(mDiffWidget, &DiffWidget::signalDiffEmpty, mControls, &Controls::disableDiff);
   connect(mDiffWidget, &DiffWidget::signalDiffEmpty, this, &GitQlientRepo::showPreviousView);
   connect(mDiffWidget, &DiffWidget::signalEditFile, this, &GitQlientRepo::signalEditFile);

   connect(mBlameWidget, &BlameWidget::showFileDiff, this, &GitQlientRepo::loadFileDiff);
   connect(mBlameWidget, &BlameWidget::signalOpenDiff, this, &GitQlientRepo::openCommitCompareDiff);

   connect(mMergeWidget, &MergeWidget::signalMergeFinished, this, &GitQlientRepo::showHistoryView);
   connect(mMergeWidget, &MergeWidget::signalMergeFinished, this, &GitQlientRepo::updateCache);
   connect(mMergeWidget, &MergeWidget::signalMergeFinished, mControls, &Controls::disableMergeWarning);
   connect(mMergeWidget, &MergeWidget::signalEditFile, this, &GitQlientRepo::signalEditFile);

   connect(mJenkins, &JenkinsWidget::gotoBranch, this, &GitQlientRepo::focusHistoryOnBranch);
   connect(mJenkins, &JenkinsWidget::gotoPullRequest, this, &GitQlientRepo::focusHistoryOnPr);

   connect(mGitLoader.data(), &GitRepoLoader::signalLoadingStarted, this, &GitQlientRepo::createProgressDialog);
   connect(mGitLoader.data(), &GitRepoLoader::signalLoadingFinished, this, &GitQlientRepo::onRepoLoadFinished);

   m_loaderThread = new QThread();
   mGitLoader->moveToThread(m_loaderThread);
   connect(this, &GitQlientRepo::signalLoadRepo, mGitLoader.data(), &GitRepoLoader::loadRepository);
   m_loaderThread->start();

   mGitLoader->setShowAll(settings.localValue(mGitBase->getGitQlientSettingsDir(), "ShowAllBranches", true).toBool());
}

GitQlientRepo::~GitQlientRepo()
{
   delete mAutoFetch;
   delete mAutoFilesUpdate;
   delete mGitWatcher;

   m_loaderThread->exit();
   m_loaderThread->deleteLater();
}

void GitQlientRepo::updateCache()
{
   if (!mCurrentDir.isEmpty())
   {
      QLog_Debug("UI", QString("Updating the GitQlient UI"));

      emit signalLoadRepo();

      mDiffWidget->reload();
   }
}

void GitQlientRepo::updateUiFromWatcher()
{
   QLog_Info("UI", QString("Updating the GitQlient UI from watcher"));

   mGitLoader->updateWipRevision();

   mHistoryWidget->updateUiFromWatcher();

   mDiffWidget->reload();
}

void GitQlientRepo::setRepository(const QString &newDir)
{
   if (!newDir.isEmpty())
   {
      QLog_Info("UI", QString("Loading repository at {%1}...").arg(newDir));

      mGitLoader->cancelAll();

      emit signalLoadRepo();

      mCurrentDir = newDir;
      clearWindow();
      setWidgetsEnabled(false);
   }
   else
   {
      QLog_Info("UI", QString("Repository is empty. Cleaning GitQlient"));

      mCurrentDir = "";
      clearWindow();
      setWidgetsEnabled(false);
   }
}

void GitQlientRepo::setWatcher()
{
   mGitWatcher = new QFileSystemWatcher(this);
   connect(mGitWatcher, &QFileSystemWatcher::fileChanged, this, [this](const QString &path) {
      if (!path.endsWith(".autosave") && !path.endsWith(".tmp") && !path.endsWith(".user"))
         updateUiFromWatcher();
   });

   QLog_Info("UI", QString("Setting the file watcher for dir {%1}").arg(mCurrentDir));

   mGitWatcher->addPath(mCurrentDir);

   QDirIterator it(mCurrentDir, QDirIterator::Subdirectories);
   while (it.hasNext())
   {
      const auto dir = it.next();

      if (it.fileInfo().isDir() && !dir.endsWith(".") && !dir.endsWith(".."))
         mGitWatcher->addPath(dir);
   }
}

void GitQlientRepo::clearWindow()
{
   blockSignals(true);

   mHistoryWidget->clear();
   mDiffWidget->clear();

   blockSignals(false);
}

void GitQlientRepo::setWidgetsEnabled(bool enabled)
{
   mControls->enableButtons(enabled);
   mHistoryWidget->setEnabled(enabled);
   mDiffWidget->setEnabled(enabled);
}

void GitQlientRepo::showFileHistory(const QString &fileName)
{
   mBlameWidget->showFileHistory(fileName);

   showBlameView();
}

void GitQlientRepo::createProgressDialog()
{
   if (!mWaitDlg)
   {
      mWaitDlg = new WaitingDlg(tr("Loading repository..."));
      mWaitDlg->exec();

      QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
   }
}

void GitQlientRepo::onRepoLoadFinished()
{
   if (!mIsInit)
   {
      updateTagsOnCache();

      mIsInit = true;

      mCurrentDir = mGitBase->getWorkingDir();

      emit repoOpened(mCurrentDir);

      setWidgetsEnabled(true);

      setWatcher();

      mBlameWidget->init(mCurrentDir);

      mControls->enableButtons(true);

      mAutoFilesUpdate->start();

      QScopedPointer<GitConfig> git(new GitConfig(mGitBase));

      if (!git->getGlobalUserInfo().isValid() && !git->getLocalUserInfo().isValid())
      {
         QLog_Info("UI", QString("Configuring Git..."));

         GitConfigDlg configDlg(mGitBase);

         configDlg.exec();

         QLog_Info("UI", QString("... Git configured!"));
      }

      QLog_Info("UI", "... repository loaded successfully");
   }

   const auto totalCommits = mGitQlientCache->count();

   mHistoryWidget->loadBranches();
   mHistoryWidget->onNewRevisions(totalCommits);
   mBlameWidget->onNewRevisions(totalCommits);

   if (mWaitDlg)
      mWaitDlg->close();
}

void GitQlientRepo::loadFileDiff(const QString &currentSha, const QString &previousSha, const QString &file,
                                 bool isCached)
{
   const auto loaded = mDiffWidget->loadFileDiff(currentSha, previousSha, file, isCached);

   if (loaded)
   {
      mControls->enableDiff();
      showDiffView();
   }
}

void GitQlientRepo::showHistoryView()
{
   mPreviousView = qMakePair(mControls->getCurrentSelectedButton(), mStackedLayout->currentWidget());

   mStackedLayout->setCurrentWidget(mHistoryWidget);
   mControls->toggleButton(ControlsMainViews::History);
}

void GitQlientRepo::showBlameView()
{
   mPreviousView = qMakePair(mControls->getCurrentSelectedButton(), mStackedLayout->currentWidget());

   mStackedLayout->setCurrentWidget(mBlameWidget);
   mControls->toggleButton(ControlsMainViews::Blame);
}

void GitQlientRepo::showDiffView()
{
   mPreviousView = qMakePair(mControls->getCurrentSelectedButton(), mStackedLayout->currentWidget());

   mStackedLayout->setCurrentWidget(mDiffWidget);
   mControls->toggleButton(ControlsMainViews::Diff);
}

void GitQlientRepo::showWarningMerge()
{
   showMergeView();

   const auto wipCommit = mGitQlientCache->getCommitInfo(CommitInfo::ZERO_SHA);

   QScopedPointer<GitRepoLoader> git(new GitRepoLoader(mGitBase, mGitQlientCache));
   git->updateWipRevision();

   const auto file = mGitQlientCache->getRevisionFile(CommitInfo::ZERO_SHA, wipCommit.parent(0));

   mMergeWidget->configure(file, MergeWidget::ConflictReason::Merge);
}

void GitQlientRepo::showCherryPickConflict()
{
   showMergeView();

   const auto wipCommit = mGitQlientCache->getCommitInfo(CommitInfo::ZERO_SHA);

   QScopedPointer<GitRepoLoader> git(new GitRepoLoader(mGitBase, mGitQlientCache));
   git->updateWipRevision();

   const auto files = mGitQlientCache->getRevisionFile(CommitInfo::ZERO_SHA, wipCommit.parent(0));

   mMergeWidget->configure(files, MergeWidget::ConflictReason::CherryPick);
}

void GitQlientRepo::showPullConflict()
{
   showMergeView();

   const auto wipCommit = mGitQlientCache->getCommitInfo(CommitInfo::ZERO_SHA);

   QScopedPointer<GitRepoLoader> git(new GitRepoLoader(mGitBase, mGitQlientCache));
   git->updateWipRevision();

   const auto files = mGitQlientCache->getRevisionFile(CommitInfo::ZERO_SHA, wipCommit.parent(0));

   mMergeWidget->configure(files, MergeWidget::ConflictReason::Pull);
}

void GitQlientRepo::showMergeView()
{
   mStackedLayout->setCurrentWidget(mMergeWidget);
   mControls->toggleButton(ControlsMainViews::Merge);
}

bool GitQlientRepo::configureGitServer() const
{
   bool isConfigured = false;

   if (!mGitServerWidget->isConfigured())
   {
      QScopedPointer<GitConfig> gitConfig(new GitConfig(mGitBase));
      const auto serverUrl = gitConfig->getServerUrl();
      const auto repoInfo = gitConfig->getCurrentRepoAndOwner();

      GitQlientSettings settings;
      const auto user = settings.globalValue(QString("%1/user").arg(serverUrl)).toString();
      const auto token = settings.globalValue(QString("%1/token").arg(serverUrl)).toString();

      GitServer::ConfigData data;
      data.user = user;
      data.token = token;
      data.serverUrl = serverUrl;
      data.repoInfo = repoInfo;

      isConfigured = mGitServerWidget->configure(data);
   }
   else
      isConfigured = true;

   return isConfigured;
}

void GitQlientRepo::showGitServerView()
{
   if (configureGitServer())
   {
      mStackedLayout->setCurrentWidget(mGitServerWidget);
      mControls->toggleButton(ControlsMainViews::GitServer);
   }
   else
      showPreviousView();
}

void GitQlientRepo::showGitServerPrView(int prNumber)
{
   if (configureGitServer())
   {
      showGitServerView();
      mGitServerWidget->openPullRequest(prNumber);
   }
}

void GitQlientRepo::showBuildSystemView()
{
   mJenkins->reload();
   mStackedLayout->setCurrentWidget(mJenkins);
   mControls->toggleButton(ControlsMainViews::BuildSystem);
}

void GitQlientRepo::showPreviousView()
{
   mStackedLayout->setCurrentWidget(mPreviousView.second);
   mControls->toggleButton(mPreviousView.first);
}

void GitQlientRepo::updateWip()
{
   mHistoryWidget->resetWip();
   mGitLoader.data()->updateWipRevision();
   mHistoryWidget->updateUiFromWatcher();
}

void GitQlientRepo::updateTagsOnCache()
{
   QScopedPointer<GitTags> gitTags(new GitTags(mGitBase));
   const auto remoteTags = gitTags->getRemoteTags();

   mGitQlientCache->updateTags(remoteTags);
}

void GitQlientRepo::focusHistoryOnBranch(const QString &branch)
{
   auto found = false;
   const auto fullBranch = QString("origin/%1").arg(branch);
   const auto remoteBranches = mGitQlientCache->getBranches(References::Type::RemoteBranches);

   for (const auto &remote : remoteBranches)
   {
      if (remote.second.contains(fullBranch))
      {
         found = true;
         mHistoryWidget->focusOnCommit(remote.first);
         showHistoryView();
      }
   }

   if (!found)
      QMessageBox::information(
          this, tr("Branch not found"),
          tr("The branch couldn't be found. Please, make sure you fetched and have the latest changes."));
}

void GitQlientRepo::focusHistoryOnPr(int prNumber)
{
   const auto pr = mGitServerCache->getPullRequest(prNumber);

   mHistoryWidget->focusOnCommit(pr.state.sha);
   showHistoryView();
}

void GitQlientRepo::openCommitDiff(const QString currentSha)
{
   const auto rev = mGitQlientCache->getCommitInfo(currentSha);
   const auto loaded = mDiffWidget->loadCommitDiff(currentSha, rev.parent(0));

   if (loaded)
   {
      mControls->enableDiff();

      showDiffView();
   }
}

void GitQlientRepo::openCommitCompareDiff(const QStringList &shas)
{
   const auto loaded = mDiffWidget->loadCommitDiff(shas.last(), shas.first());

   if (loaded)
   {
      mControls->enableDiff();

      showDiffView();
   }
}

void GitQlientRepo::changesCommitted(bool ok)
{
   if (ok)
   {
      updateCache();
      showHistoryView();
   }
   else
      QMessageBox::critical(this, tr("Commit error"), tr("Failed to commit changes"));
}

void GitQlientRepo::closeEvent(QCloseEvent *ce)
{
   QLog_Info("UI", QString("Closing GitQlient for repository {%1}").arg(mCurrentDir));

   mGitLoader->cancelAll();

   QWidget::closeEvent(ce);
}

void GitRepoLoader::cancelAll()
{
   emit cancelAllProcesses(QPrivateSignal());
}
