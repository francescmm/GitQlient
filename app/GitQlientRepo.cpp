#include "GitQlientRepo.h"

#include <RevisionsCache.h>
#include <Controls.h>
#include <BranchesWidget.h>
#include <CommitWidget.h>
#include <RevisionWidget.h>
#include <RepositoryModelColumns.h>
#include <RepositoryView.h>
#include <git.h>
#include <QLogger.h>
#include <FileDiffWidget.h>
#include <FullDiffWidget.h>
#include <domain.h>
#include <Revision.h>

#include <QTimer>
#include <QDirIterator>
#include <QFileSystemWatcher>
#include <QFileDialog>
#include <QMessageBox>
#include <QStackedWidget>
#include <QGridLayout>
#include <QApplication>

using namespace QLogger;

GitQlientRepo::GitQlientRepo(const QString &repo, QWidget *parent)
   : QFrame(parent)
   , mGit(new Git())
   , mRevisionsCache(new RevisionsCache(mGit))
   , mRepositoryView(new RepositoryView(mRevisionsCache, mGit))
   , commitStackedWidget(new QStackedWidget())
   , mainStackedWidget(new QStackedWidget())
   , mControls(new Controls(mGit))
   , mCommitWidget(new CommitWidget(mGit))
   , mRevisionWidget(new RevisionWidget(mGit))
   , mFullDiffWidget(new FullDiffWidget(mGit, mRevisionsCache))
   , mFileDiffWidget(new FileDiffWidget(mGit))
   , mBranchesWidget(new BranchesWidget(mGit))
   , mAutoFetch(new QTimer())
   , mAutoFilesUpdate(new QTimer())
{
   QLog_Info("UI", QString("Initializing GitQlient with repo {%1}").arg(repo));

   setObjectName("mainWindow");
   setWindowTitle("GitQlient");

   commitStackedWidget->setCurrentIndex(0);
   commitStackedWidget->addWidget(mRevisionWidget);
   commitStackedWidget->addWidget(mCommitWidget);
   commitStackedWidget->setFixedWidth(310);

   mainStackedWidget->setCurrentIndex(0);
   mainStackedWidget->addWidget(mRepositoryView);
   mainStackedWidget->addWidget(mFullDiffWidget);
   mainStackedWidget->addWidget(mFileDiffWidget);
   mainStackedWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

   const auto gridLayout = new QGridLayout(this);
   gridLayout->setSpacing(0);
   gridLayout->setContentsMargins(10, 0, 10, 10);
   gridLayout->addWidget(mControls, 0, 1);
   gridLayout->addWidget(commitStackedWidget, 1, 0);
   gridLayout->addWidget(mainStackedWidget, 1, 1);
   gridLayout->addItem(new QSpacerItem(10, 10, QSizePolicy::Fixed, QSizePolicy::Fixed), 1, 2);
   gridLayout->addWidget(mBranchesWidget, 1, 3);

   mRepositoryView->setup();
   mRevisionWidget->setup(mRepositoryView->domain());

   mAutoFetch->setInterval(mConfig.mAutoFetchSecs * 1000);
   mAutoFilesUpdate->setInterval(mConfig.mAutoFileUpdateSecs * 1000);

   connect(mAutoFetch, &QTimer::timeout, mControls, &Controls::fetchAll);
   connect(mAutoFilesUpdate, &QTimer::timeout, this, &GitQlientRepo::updateUiFromWatcher);

   connect(mControls, &Controls::signalOpenRepo, this, &GitQlientRepo::setRepository);
   connect(mControls, &Controls::signalGoBack, this, [this]() { mainStackedWidget->setCurrentIndex(0); });
   connect(mControls, &Controls::signalRepositoryUpdated, this, &GitQlientRepo::updateUi);
   connect(mControls, &Controls::signalGoToSha, mRepositoryView, &RepositoryView::focusOnCommit);
   connect(mControls, &Controls::signalGoToSha, this, &GitQlientRepo::onCommitSelected);

   connect(mBranchesWidget, &BranchesWidget::signalBranchesUpdated, this, &GitQlientRepo::updateUi);
   connect(mBranchesWidget, &BranchesWidget::signalSelectCommit, mRepositoryView, &RepositoryView::focusOnCommit);
   connect(mBranchesWidget, &BranchesWidget::signalSelectCommit, this, &GitQlientRepo::onCommitSelected);
   connect(mBranchesWidget, &BranchesWidget::signalOpenSubmodule, this, &GitQlientRepo::signalOpenSubmodule);

   connect(mRepositoryView, &RepositoryView::rebase, this, &GitQlientRepo::rebase);
   connect(mRepositoryView, &RepositoryView::merge, this, &GitQlientRepo::merge);
   connect(mRepositoryView, &RepositoryView::moveRef, this, &GitQlientRepo::moveRef);
   connect(mRepositoryView, &RepositoryView::signalViewUpdated, this, &GitQlientRepo::updateUi);
   connect(mRepositoryView, &RepositoryView::signalOpenDiff, this, &GitQlientRepo::openCommitDiff);
   connect(mRepositoryView, &RepositoryView::clicked, this,
           qOverload<const QModelIndex &>(&GitQlientRepo::onCommitClicked));
   connect(mRepositoryView, &RepositoryView::doubleClicked, this, &GitQlientRepo::openCommitDiff);
   connect(mRepositoryView, &RepositoryView::signalAmendCommit, this, &GitQlientRepo::onAmendCommit);

   connect(mCommitWidget, &CommitWidget::signalChangesCommitted, this, &GitQlientRepo::changesCommitted);
   connect(mCommitWidget, &CommitWidget::signalCheckoutPerformed, this, &GitQlientRepo::updateUiFromWatcher);
   connect(mRevisionWidget, &RevisionWidget::signalOpenFileCommit, this, &GitQlientRepo::onFileDiffRequested);

   setRepository(repo);

   // mAutoFetch->start();
   // mAutoFilesUpdate->start();
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

void GitQlientRepo::updateUi()
{
   if (!mCurrentDir.isEmpty())
   {
      QLog_Debug("UI", QString("Updating the GitQlient UI"));

      mGit->init(mCurrentDir, mRevisionsCache);

      mBranchesWidget->showBranches();

      mRepositoryView->clear(true);

      mGit->init2();

      const auto commitStackedIndex = commitStackedWidget->currentIndex();
      const auto currentSha = commitStackedIndex == 0 ? mRevisionWidget->getCurrentCommitSha() : ZERO_SHA;

      mRepositoryView->focusOnCommit(currentSha);

      if (commitStackedIndex == 1)
         mCommitWidget->init(currentSha);

      if (mainStackedWidget->currentIndex() == 1)
         openCommitDiff();
   }
}

void GitQlientRepo::updateUiFromWatcher()
{
   const auto commitStackedIndex = commitStackedWidget->currentIndex();

   if (commitStackedIndex == 1)
   {
      mGit->updateWipRevision();
      mCommitWidget->init(ZERO_SHA);

      if (mainStackedWidget->currentIndex() == 1)
         openCommitDiff();
   }
}

void GitQlientRepo::setRepository(const QString &newDir)
{
   if (!mRepositoryBusy && !newDir.isEmpty())
   {
      QLog_Info("UI", QString("Loading repository..."));

      mRepositoryBusy = true;

      const auto oldDir = mCurrentDir;

      resetWatcher(oldDir, newDir);

      bool archiveChanged;

      mGit->getBaseDir(newDir, mCurrentDir, archiveChanged);
      mGit->stop(archiveChanged);

      const auto ok = mGit->init(mCurrentDir, mRevisionsCache); // blocking call

      if (ok)
      {
         clearWindow(true);
         setWidgetsEnabled(true);

         mGit->init2();

         onCommitSelected(ZERO_SHA);
         mBranchesWidget->showBranches();

         mCommitWidget->init(ZERO_SHA);

         mainStackedWidget->setCurrentIndex(0);
         commitStackedWidget->setCurrentIndex(1);
         mControls->enableButtons(true);

         QLog_Info("UI", "... repository loaded successfully");
      }
      else
      {
         mCurrentDir = "";
         clearWindow(true);
         setWidgetsEnabled(false);
      }

      mRepositoryBusy = false;
   }
   else
   {
      QLog_Info("UI", QString("Repository is empty. Cleaning GitQlient"));

      mCurrentDir = "";
      clearWindow(true);
      setWidgetsEnabled(false);
   }
}

void GitQlientRepo::close()
{
   QWidget::close();
}

void GitQlientRepo::resetWatcher(const QString &oldDir, const QString &newDir)
{
   if (!mGitWatcher)
   {
      mGitWatcher = new QFileSystemWatcher(this);
      connect(
          mGitWatcher, &QFileSystemWatcher::fileChanged, this,
          [this](const QString &path) {
             if (!path.endsWith(".autosave") and !path.endsWith(".tmp") and !path.endsWith(".user"))
                updateUiFromWatcher();
          },
          Qt::UniqueConnection);
   }

   if (oldDir != newDir)
   {
      QLog_Info("UI", QString("Setting the file watcher for dir {%1}").arg(newDir));

      if (!oldDir.isEmpty())
         mGitWatcher->removePath(oldDir);

      mGitWatcher->addPath(newDir);

      QDirIterator it(newDir, QDirIterator::Subdirectories);
      while (it.hasNext())
      {
         const auto dir = it.next();

         if (!dir.endsWith(".") and !dir.endsWith(".."))
            mGitWatcher->addPath(dir);
      }
   }
}

void GitQlientRepo::clearWindow(bool deepClear)
{
   blockSignals(true);

   commitStackedWidget->setCurrentIndex(commitStackedWidget->currentIndex());
   mainStackedWidget->setCurrentIndex(0);

   mCommitWidget->clear();
   mRevisionWidget->clear();

   mRepositoryView->clear(deepClear);
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

void GitQlientRepo::openCommitDiff()
{
   mFullDiffWidget->onStateInfoUpdate(mRepositoryView->domain()->st);
   mainStackedWidget->setCurrentIndex(1);
}

void GitQlientRepo::changesCommitted(bool ok)
{
   if (ok)
      updateUi();
   else
      QMessageBox::critical(this, tr("Commit error"), tr("Failed to commit changes"));
}

void GitQlientRepo::onCommitClicked(const QModelIndex &index)
{
   if (mRepositoryView == dynamic_cast<RepositoryView *>(sender()))
   {
      const auto sha = mRepositoryView->data(index.row(), RepositoryModelColumns::SHA).toString();

      onCommitSelected(sha);
   }
}

void GitQlientRepo::onCommitSelected(const QString &goToSha)
{
   const auto sha = mGit->getRefSha(goToSha);

   const auto isWip = sha == ZERO_SHA;
   commitStackedWidget->setCurrentIndex(isWip);

   QLog_Info("UI", QString("Selected commit {%1}").arg(sha));

   if (isWip)
      mCommitWidget->init(sha);
   else
      mRevisionWidget->setCurrentCommitSha(sha);
}

void GitQlientRepo::onAmendCommit(const QString &sha)
{
   commitStackedWidget->setCurrentIndex(1);
   mCommitWidget->init(sha);
}

void GitQlientRepo::onFileDiffRequested(const QString &currentSha, const QString &previousSha, const QString &file)
{
   const auto fileWithModifications = mFileDiffWidget->onFileDiffRequested(currentSha, previousSha, file);

   if (fileWithModifications)
   {
      QLog_Info(
          "UI",
          QString("Requested diff for file {%1} on between commits {%2} and {%3}").arg(file, currentSha, previousSha));

      mainStackedWidget->setCurrentIndex(2);
   }
   else
      QMessageBox::information(this, tr("No modifications"), tr("There are no content modifications for this file"));
}

void GitQlientRepo::rebase(const QString &from, const QString &to, const QString &onto)
{
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

   const auto success = from.isEmpty()
       ? mGit->run(QString("git checkout -q %1").arg(to)).first && mGit->run(QString("git rebase %1").arg(onto)).first
       : mGit->run(QString("git rebase --onto %3 %1^ %2").arg(from, to, onto)).first;

   if (success)
      updateUi();

   QApplication::restoreOverrideCursor();
}

void GitQlientRepo::merge(const QStringList &shas, const QString &into)
{
   QString output;
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

   if (mGit->merge(into, shas, &output))
   {
      // git->commit !!
      updateUi();
   }
   else if (!output.isEmpty())
      QMessageBox::warning(this, "git merge failed", QString("\n\nGit says: \n\n%1").arg(output));

   QApplication::restoreOverrideCursor();
}

void GitQlientRepo::moveRef(const QString &target, const QString &toSHA)
{
   QString cmd;
   if (target.startsWith("remotes/"))
   {
      QString remote = target.section("/", 1, 1);
      QString name = target.section("/", 2);
      cmd = QString("git push -q %1 %2:%3").arg(remote, toSHA, name);
   }
   else if (target.startsWith("tags/"))
   {
      cmd = QString("git tag -f %1 %2").arg(target.section("/", 1), toSHA);
   }
   else if (!target.isEmpty())
   {
      const QString &sha = mGit->getRefSha(target, Git::BRANCH, false);
      if (!sha.isEmpty())
      {
         const QStringList &children = mGit->getChildren(sha);
         if ((children.count() == 0 || (children.count() == 1 && children.front() == ZERO_SHA)) && // no children
             mGit->getRefNames(sha, Git::ANY_REF).count() == 1 && // last ref name
             QMessageBox::question(this, "move branch",
                                   QString("This is the last reference to this branch.\n"
                                           "Do you really want to move '%1'?")
                                       .arg(target))
                 == QMessageBox::No)
            return;

         if (target == mGit->getCurrentBranchName()) // move current branch
            cmd = QString("git checkout -q -B %1 %2").arg(target, toSHA);
         else // move any other local branch
            cmd = QString("git branch -f %1 %2").arg(target, toSHA);
      }
   }

   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

   if (mGit->run(cmd).first)
      updateUi();

   QApplication::restoreOverrideCursor();
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

   mGit->stop(true);

   QWidget::closeEvent(ce);
}
