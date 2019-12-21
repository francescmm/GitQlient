#include "GitQlientRepo.h"

#include <GitQlientSettings.h>
#include <Controls.h>
#include <BranchesWidget.h>
#include <WorkInProgressWidget.h>
#include <CommitInfoWidget.h>
#include <CommitHistoryColumns.h>
#include <CommitHistoryWidget.h>
#include <CommitHistoryView.h>
#include <git.h>
#include <QLogger.h>
#include <FileDiffWidget.h>
#include <FullDiffWidget.h>
#include <FileHistoryWidget.h>
#include <CommitInfo.h>
#include <ProgressDlg.h>
#include <GitConfigDlg.h>

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
   , mRepoWidget(new CommitHistoryWidget(mGit))
   , commitStackedWidget(new QStackedWidget())
   , centerStackedWidget(new QStackedWidget())
   , mainStackedLayout(new QStackedLayout())
   , mControls(new Controls(mGit))
   , mCommitWidget(new WorkInProgressWidget(mGit))
   , mRevisionWidget(new CommitInfoWidget(mGit))
   , mFullDiffWidget(new FullDiffWidget(mGit))
   , mFileDiffWidget(new FileDiffWidget(mGit))
   , fileHistoryWidget(new FileHistoryWidget(mGit))
   , mAutoFetch(new QTimer())
   , mAutoFilesUpdate(new QTimer())
{
   QLog_Info("UI", QString("Initializing GitQlient"));

   setObjectName("mainWindow");
   setWindowTitle("GitQlient");

   commitStackedWidget->setCurrentIndex(0);
   commitStackedWidget->addWidget(mRevisionWidget);
   commitStackedWidget->addWidget(mCommitWidget);
   commitStackedWidget->setFixedWidth(310);

   centerStackedWidget->setCurrentIndex(0);
   centerStackedWidget->addWidget(mRepoWidget);
   centerStackedWidget->addWidget(mFullDiffWidget);
   centerStackedWidget->addWidget(mFileDiffWidget);
   centerStackedWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

   const auto centerLayout = new QHBoxLayout();
   centerLayout->setContentsMargins(QMargins());
   centerLayout->addWidget(commitStackedWidget);
   centerLayout->addWidget(centerStackedWidget);

   const auto centerWidget = new QFrame();
   centerWidget->setLayout(centerLayout);

   mainStackedLayout->addWidget(centerWidget);
   mainStackedLayout->addWidget(fileHistoryWidget);

   const auto gridLayout = new QGridLayout(this);
   gridLayout->setSpacing(0);
   gridLayout->setContentsMargins(10, 0, 10, 10);
   gridLayout->addWidget(mControls, 0, 1);
   gridLayout->addLayout(mainStackedLayout, 1, 0, 1, 3);

   mAutoFetch->setInterval(mConfig.mAutoFetchSecs * 1000);
   mAutoFilesUpdate->setInterval(mConfig.mAutoFileUpdateSecs * 1000);

   connect(mAutoFetch, &QTimer::timeout, mControls, &Controls::fetchAll);
   connect(mAutoFilesUpdate, &QTimer::timeout, this, &GitQlientRepo::updateUiFromWatcher);

   connect(mControls, &Controls::signalGoRepo, this, [this]() {
      centerStackedWidget->setCurrentIndex(0);
      mainStackedLayout->setCurrentIndex(0);
   });
   connect(mControls, &Controls::signalGoBlame, this, [this]() { mainStackedLayout->setCurrentIndex(1); });
   connect(mControls, &Controls::signalGoDiff, this, [this]() {
      centerStackedWidget->setCurrentIndex(1);
      mainStackedLayout->setCurrentIndex(0);
   });
   connect(mControls, &Controls::signalRepositoryUpdated, this, &GitQlientRepo::updateCache);

   connect(mRepoWidget, &CommitHistoryWidget::signalUpdateCache, this, &GitQlientRepo::updateCache);
   connect(mRepoWidget, &CommitHistoryWidget::signalOpenSubmodule, this, &GitQlientRepo::signalOpenSubmodule);
   connect(mRepoWidget, &CommitHistoryWidget::signalGoToSha, this, &GitQlientRepo::onCommitSelected);
   connect(mRepoWidget, &CommitHistoryWidget::signalViewUpdated, this, &GitQlientRepo::updateCache);
   connect(mRepoWidget, &CommitHistoryWidget::signalOpenDiff, this, &GitQlientRepo::openCommitDiff);
   connect(mRepoWidget, &CommitHistoryWidget::signalOpenCompareDiff, this, &GitQlientRepo::openCommitCompareDiff);
   connect(mRepoWidget, &CommitHistoryWidget::signalAmendCommit, this, &GitQlientRepo::onAmendCommit);

   connect(fileHistoryWidget, &FileHistoryWidget::showFileDiff, this, &GitQlientRepo::onFileDiffRequested);
   connect(fileHistoryWidget, &FileHistoryWidget::showFileDiff, this,
           [this](const QString &sha, const QString &, const QString &) {
              mainStackedLayout->setCurrentIndex(0);
              onCommitSelected(sha);
           });

   connect(mCommitWidget, &WorkInProgressWidget::signalShowDiff, this,
           [this](const QString &fileName) { onFileDiffRequested("", "", fileName); });
   connect(mCommitWidget, &WorkInProgressWidget::signalChangesCommitted, this, &GitQlientRepo::changesCommitted);
   connect(mCommitWidget, &WorkInProgressWidget::signalCheckoutPerformed, this, &GitQlientRepo::updateUiFromWatcher);
   connect(mCommitWidget, &WorkInProgressWidget::signalShowFileHistory, this, &GitQlientRepo::showFileHistory);

   connect(mRevisionWidget, &CommitInfoWidget::signalOpenFileCommit, this, &GitQlientRepo::onFileDiffRequested);
   connect(mRevisionWidget, &CommitInfoWidget::signalShowFileHistory, this, &GitQlientRepo::showFileHistory);

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

      const auto commitStackedIndex = commitStackedWidget->currentIndex();
      const auto currentSha = commitStackedIndex == 0 ? mRevisionWidget->getCurrentCommitSha() : ZERO_SHA;

      mRepoWidget->focusOnCommit(currentSha);

      if (commitStackedIndex == 1)
         mCommitWidget->configure(currentSha);

      if (centerStackedWidget->currentIndex() == 1)
         openCommitDiff();
   }
}

void GitQlientRepo::updateUiFromWatcher()
{
   const auto commitStackedIndex = commitStackedWidget->currentIndex();

   if (commitStackedIndex == 1)
   {
      QLog_Info("UI", QString("Updating the GitQlient UI from watcher"));

      mGit->updateWipRevision();

      if (!mCommitWidget->isAmendActive())
         mCommitWidget->configure(ZERO_SHA);

      if (centerStackedWidget->currentIndex() == 1)
         openCommitDiff();
   }
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

         onCommitSelected(ZERO_SHA);

         mRepoWidget->reload();

         fileHistoryWidget->init(newDir);

         centerStackedWidget->setCurrentIndex(0);
         commitStackedWidget->setCurrentIndex(1);
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

   commitStackedWidget->setCurrentIndex(commitStackedWidget->currentIndex());
   centerStackedWidget->setCurrentIndex(0);

   mCommitWidget->clear();
   mRevisionWidget->clear();

   mRepoWidget->clear();
   mFullDiffWidget->clear();
   mFileDiffWidget->clear();

   blockSignals(false);
}

void GitQlientRepo::setWidgetsEnabled(bool enabled)
{
   mControls->enableButtons(enabled);
   mCommitWidget->setEnabled(enabled);
   mRevisionWidget->setEnabled(enabled);
   commitStackedWidget->setEnabled(enabled);
   mRepoWidget->setEnabled(enabled);
   mFullDiffWidget->setEnabled(enabled);
   mFileDiffWidget->setEnabled(enabled);
}

void GitQlientRepo::showFileHistory(const QString &fileName)
{
   fileHistoryWidget->showFileHistory(fileName);
   mainStackedLayout->setCurrentIndex(1);
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

void GitQlientRepo::openCommitDiff()
{
   const auto currentSha = mRepoWidget->getCurrentSha();

   if (!(currentSha == ZERO_SHA && mGit->isNothingToCommit()))
   {
      const auto rev = mGit->getCommitInfo(currentSha);
      mFullDiffWidget->loadDiff(currentSha, rev.parent(0));
      centerStackedWidget->setCurrentIndex(1);
   }
}

void GitQlientRepo::openCommitCompareDiff(const QStringList &shas)
{
   mFullDiffWidget->loadDiff(shas.last(), shas.first());
   centerStackedWidget->setCurrentIndex(1);
}

void GitQlientRepo::changesCommitted(bool ok)
{
   if (ok)
   {
      updateCache();
      centerStackedWidget->setCurrentIndex(0);
      onCommitSelected(ZERO_SHA);
   }
   else
      QMessageBox::critical(this, tr("Commit error"), tr("Failed to commit changes"));
}

void GitQlientRepo::onCommitSelected(const QString &goToSha)
{
   const auto isWip = goToSha == ZERO_SHA;
   commitStackedWidget->setCurrentIndex(isWip);

   QLog_Info("UI", QString("Selected commit {%1}").arg(goToSha));

   if (isWip)
      mCommitWidget->configure(goToSha);
   else
      mRevisionWidget->setCurrentCommitSha(goToSha);
}

void GitQlientRepo::onAmendCommit(const QString &sha)
{
   commitStackedWidget->setCurrentIndex(1);
   mCommitWidget->configure(sha);
}

void GitQlientRepo::onFileDiffRequested(const QString &currentSha, const QString &previousSha, const QString &file)
{
   const auto fileWithModifications = mFileDiffWidget->configure(currentSha, previousSha, file);

   if (fileWithModifications)
   {
      QLog_Info(
          "UI",
          QString("Requested diff for file {%1} on between commits {%2} and {%3}").arg(file, currentSha, previousSha));

      centerStackedWidget->setCurrentIndex(2);
   }
   else
      QMessageBox::information(this, tr("No modifications"), tr("There are no content modifications for this file"));
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
