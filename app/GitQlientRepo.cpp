#include "GitQlientRepo.h"

#include <Controls.h>
#include <BranchesWidget.h>
#include <WorkInProgressWidget.h>
#include <CommitInfoWidget.h>
#include <RepositoryViewDelegate.h>
#include <CommitHistoryColumns.h>
#include <CommitHistoryModel.h>
#include <CommitHistoryView.h>
#include <git.h>
#include <QLogger.h>
#include <FileDiffWidget.h>
#include <FullDiffWidget.h>
#include <FileHistoryWidget.h>
#include <CommitInfo.h>

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

GitQlientRepo::GitQlientRepo(const QString &repo, QWidget *parent)
   : QFrame(parent)
   , mGit(new Git())
   , mRepositoryModel(new CommitHistoryModel(mGit))
   , mRepositoryView(new CommitHistoryView(mGit))
   , commitStackedWidget(new QStackedWidget())
   , centerStackedWidget(new QStackedWidget())
   , mainStackedLayout(new QStackedLayout())
   , mControls(new Controls(mGit))
   , mCommitWidget(new WorkInProgressWidget(mGit))
   , mRevisionWidget(new CommitInfoWidget(mGit))
   , mFullDiffWidget(new FullDiffWidget(mGit))
   , mFileDiffWidget(new FileDiffWidget(mGit))
   , mBranchesWidget(new BranchesWidget(mGit))
   , fileHistoryWidget(new FileHistoryWidget(mGit))

   , mAutoFetch(new QTimer())
   , mAutoFilesUpdate(new QTimer())
{
   QLog_Info("UI", QString("Initializing GitQlient with repo {%1}").arg(repo));

   setObjectName("mainWindow");
   setWindowTitle("GitQlient");

   mRepositoryView->setObjectName("mainRepoView");
   mRepositoryView->setModel(mRepositoryModel);
   mRepositoryView->setItemDelegate(new RepositoryViewDelegate(mGit, mRepositoryView));

   commitStackedWidget->setCurrentIndex(0);
   commitStackedWidget->addWidget(mRevisionWidget);
   commitStackedWidget->addWidget(mCommitWidget);
   commitStackedWidget->setFixedWidth(310);

   centerStackedWidget->setCurrentIndex(0);
   centerStackedWidget->addWidget(mRepositoryView);
   centerStackedWidget->addWidget(mFullDiffWidget);
   centerStackedWidget->addWidget(mFileDiffWidget);
   centerStackedWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

   const auto centerLayout = new QHBoxLayout();
   centerLayout->setContentsMargins(QMargins());
   centerLayout->addWidget(commitStackedWidget);
   centerLayout->addWidget(centerStackedWidget);
   centerLayout->addItem(new QSpacerItem(10, 10, QSizePolicy::Fixed, QSizePolicy::Fixed));
   centerLayout->addWidget(mBranchesWidget);

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

   connect(mControls, &Controls::signalGoBack, this, [this]() {
      centerStackedWidget->setCurrentIndex(0);
      mainStackedLayout->setCurrentIndex(0);
   });
   connect(mControls, &Controls::signalGoBlame, this, [this]() { mainStackedLayout->setCurrentIndex(1); });
   connect(mControls, &Controls::signalRepositoryUpdated, this, &GitQlientRepo::updateCache);
   connect(mControls, &Controls::signalGoToSha, mRepositoryView, &CommitHistoryView::focusOnCommit);
   connect(mControls, &Controls::signalGoToSha, this, &GitQlientRepo::onCommitSelected);

   connect(mBranchesWidget, &BranchesWidget::signalBranchesUpdated, this, &GitQlientRepo::updateCache);
   connect(mBranchesWidget, &BranchesWidget::signalBranchCheckedOut, this, &GitQlientRepo::updateCache);
   connect(mBranchesWidget, &BranchesWidget::signalSelectCommit, mRepositoryView, &CommitHistoryView::focusOnCommit);
   connect(mBranchesWidget, &BranchesWidget::signalSelectCommit, this, &GitQlientRepo::onCommitSelected);
   connect(mBranchesWidget, &BranchesWidget::signalOpenSubmodule, this, &GitQlientRepo::signalOpenSubmodule);

   connect(mRepositoryView, &CommitHistoryView::signalViewUpdated, this, &GitQlientRepo::updateCache);
   connect(mRepositoryView, &CommitHistoryView::signalOpenDiff, this, &GitQlientRepo::openCommitDiff);
   connect(mRepositoryView, &CommitHistoryView::signalOpenCompareDiff, this, &GitQlientRepo::openCommitCompareDiff);
   connect(mRepositoryView, &CommitHistoryView::clicked, this, &GitQlientRepo::onCommitClicked);
   connect(mRepositoryView, &CommitHistoryView::doubleClicked, this, &GitQlientRepo::openCommitDiff);
   connect(mRepositoryView, &CommitHistoryView::signalAmendCommit, this, &GitQlientRepo::onAmendCommit);

   connect(mCommitWidget, &WorkInProgressWidget::signalChangesCommitted, this, &GitQlientRepo::changesCommitted);
   connect(mCommitWidget, &WorkInProgressWidget::signalCheckoutPerformed, this, &GitQlientRepo::updateUiFromWatcher);
   connect(mCommitWidget, &WorkInProgressWidget::signalShowFileHistory, this, &GitQlientRepo::showFileHistory);

   connect(mRevisionWidget, &CommitInfoWidget::signalOpenFileCommit, this, &GitQlientRepo::onFileDiffRequested);
   connect(mRevisionWidget, &CommitInfoWidget::signalShowFileHistory, this, &GitQlientRepo::showFileHistory);

   setRepository(repo);

   mAutoFilesUpdate->start();
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

      mGit->loadRepository(mCurrentDir);

      mBranchesWidget->showBranches();

      mRepositoryView->clear();

      mGit->init2();

      const auto commitStackedIndex = commitStackedWidget->currentIndex();
      const auto currentSha = commitStackedIndex == 0 ? mRevisionWidget->getCurrentCommitSha() : ZERO_SHA;

      mRepositoryView->focusOnCommit(currentSha);

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
      QLog_Info("UI", QString("Loading repository..."));

      emit mGit->cancelAllProcesses();

      const auto ok = mGit->loadRepository(newDir);
      mCurrentDir = mGit->getWorkingDir();

      if (ok)
      {
         setWidgetsEnabled(true);

         mGit->init2();

         setWatcher();

         onCommitSelected(ZERO_SHA);
         mBranchesWidget->showBranches();

         fileHistoryWidget->init(newDir);

         centerStackedWidget->setCurrentIndex(0);
         commitStackedWidget->setCurrentIndex(1);
         mControls->enableButtons(true);

         QLog_Info("UI", "... repository loaded successfully");
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

   mRepositoryView->clear();
   mFullDiffWidget->clear();
   mFileDiffWidget->clear();
   mBranchesWidget->clear();

   blockSignals(false);
}

void GitQlientRepo::setWidgetsEnabled(bool enabled)
{
   mControls->enableButtons(enabled);
   mCommitWidget->setEnabled(enabled);
   mRevisionWidget->setEnabled(enabled);
   commitStackedWidget->setEnabled(enabled);
   mRepositoryView->setEnabled(enabled);
   mFullDiffWidget->setEnabled(enabled);
   mFileDiffWidget->setEnabled(enabled);
   mBranchesWidget->setEnabled(enabled);
}

void GitQlientRepo::showFileHistory(const QString &fileName)
{
   fileHistoryWidget->showFileHistory(fileName);
   mainStackedLayout->setCurrentIndex(1);
}

void GitQlientRepo::openCommitDiff()
{
   const auto currentSha = mRepositoryView->getCurrentSha();
   const auto rev = mGit->getCommitInfo(currentSha);
   mFullDiffWidget->loadDiff(currentSha, rev.parent(0));
   centerStackedWidget->setCurrentIndex(1);
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

void GitQlientRepo::onCommitClicked(const QModelIndex &index)
{
   const auto sha = mGit->getCommitInfoByRow(index.row()).sha();

   onCommitSelected(sha);
}

void GitQlientRepo::onCommitSelected(const QString &goToSha)
{
   const auto sha = mGit->getRefSha(goToSha);

   const auto isWip = sha == ZERO_SHA;
   commitStackedWidget->setCurrentIndex(isWip);

   QLog_Info("UI", QString("Selected commit {%1}").arg(sha));

   if (isWip)
      mCommitWidget->configure(sha);
   else
      mRevisionWidget->setCurrentCommitSha(sha);
}

void GitQlientRepo::onAmendCommit(const QString &sha)
{
   commitStackedWidget->setCurrentIndex(1);
   mCommitWidget->configure(sha);
}

void GitQlientRepo::onFileDiffRequested(const QString &currentSha, const QString &previousSha, const QString &file)
{
   const auto fileWithModifications = mFileDiffWidget->onFileDiffRequested(currentSha, previousSha, file);

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
