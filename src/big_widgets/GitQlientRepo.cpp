#include "GitQlientRepo.h"

#include <GitQlientSettings.h>
#include <Controls.h>
#include <BranchesWidget.h>
#include <CommitHistoryColumns.h>
#include <HistoryWidget.h>
#include <QLogger.h>
#include <BlameWidget.h>
#include <CommitInfo.h>
#include <ProgressDlg.h>
#include <GitConfigDlg.h>
#include <Controls.h>
#include <HistoryWidget.h>
#include <DiffWidget.h>
#include <MergeWidget.h>
#include <RevisionsCache.h>
#include <GitRepoLoader.h>
#include <GitConfig.h>
#include <GitBase.h>
#include <GitHistory.h>

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

GitQlientRepo::GitQlientRepo(const QString &repoPath, QWidget *parent)
   : QFrame(parent)
   , mGitQlientCache(new RevisionsCache())
   , mGitBase(new GitBase(repoPath))
   , mGitLoader(new GitRepoLoader(mGitBase, mGitQlientCache))
   , mHistoryWidget(new HistoryWidget(mGitQlientCache, mGitBase))
   , mStackedLayout(new QStackedLayout())
   , mControls(new Controls(mGitBase))
   , mDiffWidget(new DiffWidget(mGitBase, mGitQlientCache))
   , mBlameWidget(new BlameWidget(mGitQlientCache, mGitBase))
   , mMergeWidget(new MergeWidget(mGitQlientCache, mGitBase))
   , mAutoFetch(new QTimer())
   , mAutoFilesUpdate(new QTimer())
{
   setAttribute(Qt::WA_DeleteOnClose);

   QLog_Info("UI", QString("Initializing GitQlient"));

   setObjectName("mainWindow");
   setWindowTitle("GitQlient");

   mStackedLayout->addWidget(mHistoryWidget);
   mStackedLayout->addWidget(mDiffWidget);
   mStackedLayout->addWidget(mBlameWidget);
   mStackedLayout->addWidget(mMergeWidget);
   showHistoryView();

   const auto mainLayout = new QVBoxLayout(this);
   mainLayout->setSpacing(0);
   mainLayout->setContentsMargins(10, 0, 10, 10);
   mainLayout->addWidget(mControls);
   mainLayout->addLayout(mStackedLayout);

   mAutoFetch->setInterval(mConfig.mAutoFetchSecs * 1000);
   mAutoFilesUpdate->setInterval(mConfig.mAutoFileUpdateSecs * 1000);

   connect(mAutoFetch, &QTimer::timeout, mControls, &Controls::fetchAll);
   connect(mAutoFilesUpdate, &QTimer::timeout, this, &GitQlientRepo::updateUiFromWatcher);

   connect(mControls, &Controls::signalGoRepo, this, &GitQlientRepo::showHistoryView);
   connect(mControls, &Controls::signalGoBlame, this, &GitQlientRepo::showBlameView);
   connect(mControls, &Controls::signalGoDiff, this, &GitQlientRepo::showDiffView);
   connect(mControls, &Controls::signalGoMerge, this, &GitQlientRepo::showMergeView);
   connect(mControls, &Controls::signalRepositoryUpdated, this, &GitQlientRepo::updateCache);
   connect(mControls, &Controls::signalPullConflict, mControls, &Controls::activateMergeWarning);
   connect(mControls, &Controls::signalPullConflict, this, &GitQlientRepo::showPullConflict);

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
   connect(mHistoryWidget, &HistoryWidget::signalPullConflict, this, &GitQlientRepo::showPullConflict);
   connect(mHistoryWidget, &HistoryWidget::signalUpdateWip, this, &GitQlientRepo::updateWip);

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

   connect(mGitLoader.data(), &GitRepoLoader::signalLoadingStarted, this, &GitQlientRepo::updateProgressDialog,
           Qt::DirectConnection);
   connect(mGitLoader.data(), &GitRepoLoader::signalLoadingFinished, this, &GitQlientRepo::onRepoLoadFinished,
           Qt::DirectConnection);

   GitQlientSettings settings;
   mGitLoader->setShowAll(settings.value("ShowAllBranches", true).toBool());

   setRepository(repoPath);
}

GitQlientRepo::~GitQlientRepo()
{
   delete mAutoFetch;
   delete mAutoFilesUpdate;
   delete mGitWatcher;
}

void GitQlientRepo::setConfig(const GitQlientRepoConfig &config)
{
   QLog_Debug("UI", QString("Setting GitQlientRepo configuration."));

   mConfig = config;

   mAutoFetch->stop();
   mAutoFetch->setInterval(mConfig.mAutoFetchSecs);
   mAutoFetch->start();

   mAutoFilesUpdate->stop();
   mAutoFilesUpdate->setInterval(mConfig.mAutoFileUpdateSecs);
   mAutoFilesUpdate->start();
}

void GitQlientRepo::updateCache()
{
   if (!mCurrentDir.isEmpty())
   {
      QLog_Debug("UI", QString("Updating the GitQlient UI"));

      mHistoryWidget->clear();

      mGitLoader->loadRepository();

      mHistoryWidget->reload();

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

      const auto ok = mGitLoader->loadRepository();

      if (ok)
      {
         GitQlientSettings settings;
         settings.setProjectOpened(newDir);

         mCurrentDir = mGitBase->getWorkingDir();
         setWidgetsEnabled(true);

         setWatcher();

         mHistoryWidget->reload();

         mBlameWidget->init(newDir);

         mControls->enableButtons(true);

         mAutoFilesUpdate->start();

         QScopedPointer<GitConfig> git(new GitConfig(mGitBase));

         if (!git->getGlobalUserInfo().isValid())
         {
            QLog_Info("UI", QString("Configuring Git..."));

            GitConfigDlg configDlg(mGitBase);

            configDlg.exec();

            QLog_Info("UI", QString("... Git configured!"));
         }

         QLog_Info("UI", "... repository loaded successfully");
      }
      else
      {
         QLog_Error("Git", QString("There was an error during the repository load!"));

         mCurrentDir = "";
         clearWindow();
         setWidgetsEnabled(false);
      }
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

void GitQlientRepo::updateProgressDialog()
{
   if (!mProgressDlg)
   {
      mProgressDlg = new ProgressDlg(tr("Loading repository..."), QString(), 0, 0, false, true);
      connect(mProgressDlg, &ProgressDlg::destroyed, this, [this]() { mProgressDlg = nullptr; });

      mProgressDlg->show();
   }
}

void GitQlientRepo::onRepoLoadFinished()
{
   mProgressDlg->close();

   const auto totalCommits = mGitQlientCache->count();

   mHistoryWidget->onNewRevisions(totalCommits);
   mBlameWidget->onNewRevisions(totalCommits);
}

void GitQlientRepo::loadFileDiff(const QString &currentSha, const QString &previousSha, const QString &file)
{
   const auto loaded = mDiffWidget->loadFileDiff(currentSha, previousSha, file);

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
   mControls->toggleButton(ControlsMainViews::HISTORY);
}

void GitQlientRepo::showBlameView()
{
   mPreviousView = qMakePair(mControls->getCurrentSelectedButton(), mStackedLayout->currentWidget());

   mStackedLayout->setCurrentWidget(mBlameWidget);
   mControls->toggleButton(ControlsMainViews::BLAME);
}

void GitQlientRepo::showDiffView()
{
   mPreviousView = qMakePair(mControls->getCurrentSelectedButton(), mStackedLayout->currentWidget());

   mStackedLayout->setCurrentWidget(mDiffWidget);
   mControls->toggleButton(ControlsMainViews::DIFF);
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
   mControls->toggleButton(ControlsMainViews::MERGE);
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

void GitQlientRepo::openCommitDiff(const QString currentSha)
{
   const auto rev = mGitQlientCache->getCommitInfo(currentSha);

   mDiffWidget->loadCommitDiff(currentSha, rev.parent(0));
   mControls->enableDiff();

   showDiffView();
}

void GitQlientRepo::openCommitCompareDiff(const QStringList &shas)
{
   mDiffWidget->loadCommitDiff(shas.last(), shas.first());
   mControls->enableDiff();
   showDiffView();
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
