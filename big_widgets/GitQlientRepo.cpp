#include "GitQlientRepo.h"

#include <GitQlientSettings.h>
#include <Controls.h>
#include <BranchesWidget.h>
#include <CommitHistoryColumns.h>
#include <HistoryWidget.h>
#include <CommitHistoryView.h>
#include <QLogger.h>
#include <BlameWidget.h>
#include <CommitInfo.h>
#include <ProgressDlg.h>
#include <GitConfigDlg.h>
#include <DiffWidget.h>
#include <RevisionsCache.h>

#include <GitRepoLoader.h>
#include <GitConfig.h>
#include <GitBase.h>

#include <QFileSystemModel>
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
   , mAutoFetch(new QTimer())
   , mAutoFilesUpdate(new QTimer())
{
   QLog_Info("UI", QString("Initializing GitQlient"));

   setObjectName("mainWindow");
   setWindowTitle("GitQlient");

   mStackedLayout->addWidget(mHistoryWidget);
   mStackedLayout->addWidget(mDiffWidget);
   mStackedLayout->addWidget(mBlameWidget);
   showHistoryView();

   const auto gridLayout = new QGridLayout(this);
   gridLayout->setSpacing(0);
   gridLayout->setContentsMargins(10, 0, 10, 10);
   gridLayout->addWidget(mControls, 0, 1);
   gridLayout->addLayout(mStackedLayout, 1, 0, 1, 3);

   mAutoFetch->setInterval(mConfig.mAutoFetchSecs * 1000);
   mAutoFilesUpdate->setInterval(mConfig.mAutoFileUpdateSecs * 1000);

   connect(mAutoFetch, &QTimer::timeout, mControls, &Controls::fetchAll);
   connect(mAutoFilesUpdate, &QTimer::timeout, this, &GitQlientRepo::updateUiFromWatcher);

   connect(mControls, &Controls::signalGoRepo, this, &GitQlientRepo::showHistoryView);
   connect(mControls, &Controls::signalGoBlame, this, &GitQlientRepo::showBlameView);
   connect(mControls, &Controls::signalGoDiff, this, &GitQlientRepo::showDiffView);
   connect(mControls, &Controls::signalRepositoryUpdated, this, &GitQlientRepo::updateCache);

   connect(mHistoryWidget, &HistoryWidget::signalAllBranchesActive, mGitLoader.get(), &GitRepoLoader::setShowAll);
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
   connect(mHistoryWidget, &HistoryWidget::signalOpenFileCommit, this, &GitQlientRepo::loadFileDiff);

   connect(mBlameWidget, &BlameWidget::showFileDiff, this, &GitQlientRepo::loadFileDiff);

   connect(mGitLoader.get(), &GitRepoLoader::signalLoadingStarted, this, &GitQlientRepo::updateProgressDialog,
           Qt::DirectConnection);
   connect(mGitLoader.get(), &GitRepoLoader::signalLoadingFinished, this, &GitQlientRepo::onRepoLoadFinished,
           Qt::DirectConnection);
   /*
   connect(mGit.get(), &Git::signalWipUpdated, mGitLoader.get(), &GitRepoLoader::updateWipRevision,
           Qt::DirectConnection);
*/
   GitQlientSettings settings;
   mGitLoader->setShowAll(settings.value("ShowAllBranches", true).toBool());

   setRepository(repoPath);
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

         emit signalRepoOpened();

         mCurrentDir = mGitBase->getWorkingDir();
         setWidgetsEnabled(true);

         setWatcher();

         mHistoryWidget->onCommitSelected(CommitInfo::ZERO_SHA);

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

void GitQlientRepo::close()
{
   QWidget::close();
}

void GitQlientRepo::setWatcher()
{
   const auto gitWatcher = new QFileSystemWatcher(this);
   connect(gitWatcher, &QFileSystemWatcher::fileChanged, this, [this](const QString &path) {
      if (!path.endsWith(".autosave") and !path.endsWith(".tmp") and !path.endsWith(".user"))
         updateUiFromWatcher();
   });

   QLog_Info("UI", QString("Setting the file watcher for dir {%1}").arg(mCurrentDir));

   gitWatcher->addPath(mCurrentDir);

   QDirIterator it(mCurrentDir, QDirIterator::Subdirectories);
   while (it.hasNext())
   {
      const auto dir = it.next();

      if (it.fileInfo().isDir() and !dir.endsWith(".") and !dir.endsWith(".."))
         gitWatcher->addPath(dir);
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
      showDiffView();
}

void GitQlientRepo::showHistoryView()
{
   mStackedLayout->setCurrentWidget(mHistoryWidget);
   mControls->toggleButton(ControlsMainViews::HISTORY);
}

void GitQlientRepo::showBlameView()
{
   mStackedLayout->setCurrentWidget(mBlameWidget);
   mControls->toggleButton(ControlsMainViews::BLAME);
}

void GitQlientRepo::showDiffView()
{
   mStackedLayout->setCurrentWidget(mDiffWidget);
   mControls->toggleButton(ControlsMainViews::DIFF);
}

void GitQlientRepo::openCommitDiff()
{
   const auto currentSha = mHistoryWidget->getCurrentSha();
   const auto rev = mGitQlientCache->getCommitInfo(currentSha);

   mDiffWidget->loadCommitDiff(currentSha, rev.parent(0));

   showDiffView();
}

void GitQlientRepo::openCommitCompareDiff(const QStringList &shas)
{
   mDiffWidget->loadCommitDiff(shas.last(), shas.first());

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

   emit closeAllWindows();

   hide();

   mGitLoader->cancelAll();

   QWidget::closeEvent(ce);
}
