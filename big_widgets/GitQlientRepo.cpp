#include "GitQlientRepo.h"

#include <GitQlientSettings.h>
#include <Controls.h>
#include <BranchesWidget.h>
#include <CommitHistoryColumns.h>
#include <HistoryWidget.h>
#include <CommitHistoryView.h>
#include <git.h>
#include <QLogger.h>
#include <BlameWidget.h>
#include <CommitInfo.h>
#include <ProgressDlg.h>
#include <GitConfigDlg.h>
#include <DiffWidget.h>

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

GitQlientRepo::GitQlientRepo(QWidget *parent)
   : QFrame(parent)
   , mGit(new Git())
   , mRepoWidget(new HistoryWidget(mGit))
   , mStackedLayout(new QStackedLayout())
   , mControls(new Controls(mGit))
   , mDiffWidget(new DiffWidget(mGit))
   , mBlameWidget(new BlameWidget(mGit))
   , mAutoFetch(new QTimer())
   , mAutoFilesUpdate(new QTimer())
{
   QLog_Info("UI", QString("Initializing GitQlient"));

   setObjectName("mainWindow");
   setWindowTitle("GitQlient");

   mStackedLayout->addWidget(mRepoWidget);
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

   connect(mRepoWidget, &HistoryWidget::signalUpdateCache, this, &GitQlientRepo::updateCache);
   connect(mRepoWidget, &HistoryWidget::signalOpenSubmodule, this, &GitQlientRepo::signalOpenSubmodule);
   connect(mRepoWidget, &HistoryWidget::signalViewUpdated, this, &GitQlientRepo::updateCache);
   connect(mRepoWidget, &HistoryWidget::signalOpenDiff, this, &GitQlientRepo::openCommitDiff);
   connect(mRepoWidget, &HistoryWidget::signalOpenDiff, this, &GitQlientRepo::showDiffView);
   connect(mRepoWidget, &HistoryWidget::signalOpenCompareDiff, this, &GitQlientRepo::openCommitCompareDiff);
   connect(mRepoWidget, &HistoryWidget::signalShowDiff, this, &GitQlientRepo::loadFileDiff);
   connect(mRepoWidget, &HistoryWidget::signalChangesCommitted, this, &GitQlientRepo::changesCommitted);
   connect(mRepoWidget, &HistoryWidget::signalUpdateUi, this, &GitQlientRepo::updateUiFromWatcher);
   connect(mRepoWidget, &HistoryWidget::signalShowFileHistory, this, &GitQlientRepo::showFileHistory);
   connect(mRepoWidget, &HistoryWidget::signalOpenFileCommit, this, &GitQlientRepo::loadFileDiff);

   connect(mBlameWidget, &BlameWidget::showFileDiff, this, &GitQlientRepo::loadFileDiff);

   connect(mGit.get(), &Git::signalLoadingStarted, this, &GitQlientRepo::updateProgressDialog, Qt::DirectConnection);
   connect(mGit.get(), &Git::signalLoadingFinished, this, &GitQlientRepo::closeProgressDialog, Qt::DirectConnection);
}

void GitQlientRepo::setConfig(const GitQlientRepoConfig &config)
{
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

      mRepoWidget->clear();

      mGit->loadRepository(mCurrentDir);

      mRepoWidget->reload();

      mDiffWidget->reload();
   }
}

void GitQlientRepo::updateUiFromWatcher()
{
   QLog_Info("UI", QString("Updating the GitQlient UI from watcher"));

   mGit->updateWipRevision();

   mRepoWidget->updateUiFromWatcher();

   mDiffWidget->reload();
}

void GitQlientRepo::setRepository(const QString &newDir)
{
   if (!newDir.isEmpty())
   {
      QLog_Info("UI", QString("Loading repository at {%1}...").arg(newDir));

      emit mGit->cancelAllProcesses();

      const auto ok = mGit->loadRepository(newDir);

      if (ok)
      {
         GitQlientSettings settings;
         settings.setProjectOpened(newDir);

         emit signalRepoOpened();

         mCurrentDir = mGit->getWorkingDir();
         setWidgetsEnabled(true);

         setWatcher();

         mRepoWidget->onCommitSelected(ZERO_SHA);

         mRepoWidget->reload();

         mBlameWidget->init(newDir);

         mControls->enableButtons(true);

         mAutoFilesUpdate->start();

         QLog_Info("UI", "... repository loaded successfully");

         if (!mGit->getGlobalUserInfo().isValid())
         {
            QLog_Info("UI", QString("Configuring Git..."));

            GitConfigDlg configDlg(mGit);

            configDlg.exec();

            QLog_Info("UI", QString("... Git configured!"));
         }
      }
      else
      {
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
   mGitWatcher = new QFileSystemWatcher(this);
   connect(mGitWatcher, &QFileSystemWatcher::fileChanged, this, [this](const QString &path) {
      if (!path.endsWith(".autosave") and !path.endsWith(".tmp") and !path.endsWith(".user"))
         updateUiFromWatcher();
   });

   QLog_Info("UI", QString("Setting the file watcher for dir {%1}").arg(mCurrentDir));

   mGitWatcher->addPath(mCurrentDir);

   QDirIterator it(mCurrentDir, QDirIterator::Subdirectories);
   while (it.hasNext())
   {
      const auto dir = it.next();

      if (it.fileInfo().isDir() and !dir.endsWith(".") and !dir.endsWith(".."))
         mGitWatcher->addPath(dir);
   }
}

void GitQlientRepo::clearWindow()
{
   blockSignals(true);

   mRepoWidget->clear();
   mDiffWidget->clear();

   blockSignals(false);
}

void GitQlientRepo::setWidgetsEnabled(bool enabled)
{
   mControls->enableButtons(enabled);
   mRepoWidget->setEnabled(enabled);
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

void GitQlientRepo::closeProgressDialog()
{
   mProgressDlg->close();
}

void GitQlientRepo::loadFileDiff(const QString &currentSha, const QString &previousSha, const QString &file)
{
   const auto loaded = mDiffWidget->loadFileDiff(currentSha, previousSha, file);

   if (loaded)
      showDiffView();
}

void GitQlientRepo::showHistoryView()
{
   mStackedLayout->setCurrentWidget(mRepoWidget);
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
   const auto currentSha = mRepoWidget->getCurrentSha();

   if (!(currentSha == ZERO_SHA && mGit->isNothingToCommit()))
   {
      const auto rev = mGit->getCommitInfo(currentSha);
      mDiffWidget->loadCommitDiff(currentSha, rev.parent(0));
   }
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

   // lastWindowClosed() signal is emitted by close(), after sending
   // closeEvent(), so we need to close _here_ all secondary windows before
   // the close() method checks for lastWindowClosed flag to avoid missing
   // the signal and stay in the main loop forever, because lastWindowClosed()
   // signal is connected to qApp->quit()
   //
   // note that we cannot rely on setting 'this' parent in secondary windows
   // because when close() is called children are still alive and, finally,
   // when children are deleted, d'tor do not call close() anymore. So we miss
   // lastWindowClosed() signal in this case.
   emit closeAllWindows();
   hide();

   emit mGit->cancelAllProcesses();

   QWidget::closeEvent(ce);
}
