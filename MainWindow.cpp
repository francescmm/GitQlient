#include "MainWindow.h"
#include <ui_MainWindow.h>

#include <RepositoryModel.h>
#include <RepositoryView.h>
#include <git.h>
#include <revsview.h>
#include <QLogger.h>

#include <QDirIterator>
#include <QFileSystemWatcher>
#include <QFileDialog>
#include <QMessageBox>

using namespace QLogger;

MainWindow::MainWindow(QWidget *p)
   : QFrame(p)
   , ui(new Ui::MainWindow)
   , rv(new RevsView(true))
   , mGit(Git::getInstance())
{
   setObjectName("mainWindow");
   setWindowTitle("GitQlient");

   QFile styles(":/stylesheet.css");

   if (styles.open(QIODevice::ReadOnly))
   {
      setStyleSheet(QString::fromUtf8(styles.readAll()));
      styles.close();
   }

   ui->setupUi(this);
   ui->mainStackedWidget->setCurrentIndex(0);
   ui->page_5->layout()->addWidget(rv->getRepoList());
   ui->controls->enableButtons(false);

   ui->revisionWidget->setup(rv);

   qApp->installEventFilter(this);

   rv->setEnabled(false);

   connect(ui->controls, &Controls::signalOpenRepo, this, &MainWindow::setRepository);
   connect(ui->controls, &Controls::signalGoBack, this, [this]() { ui->mainStackedWidget->setCurrentIndex(0); });
   connect(ui->controls, &Controls::signalRepositoryUpdated, this, &MainWindow::updateUi);
   connect(ui->controls, &Controls::signalGoToSha, this, &MainWindow::goToCommitSha);

   connect(ui->branchesWidget, &BranchesWidget::signalBranchesUpdated, this, &MainWindow::updateUi);
   connect(ui->branchesWidget, &BranchesWidget::signalSelectCommit, this, &MainWindow::goToCommitSha);

   connect(rv, &RevsView::rebase, this, &MainWindow::rebase);
   connect(rv, &RevsView::merge, this, &MainWindow::merge);
   connect(rv, &RevsView::moveRef, this, &MainWindow::moveRef);
   connect(rv, &RevsView::signalViewUpdated, this, &MainWindow::updateUi);
   connect(rv, &RevsView::signalOpenDiff, this, &MainWindow::openCommitDiff);

   connect(ui->revisionWidget, &RevisionWidget::signalOpenFileContextMenu, rv, &RevsView::on_contextMenu);
   connect(ui->revisionWidget, &RevisionWidget::signalOpenFileCommit, this, &MainWindow::onFileDiffRequested);

   connect(ui->commitWidget, &CommitWidget::signalChangesCommitted, this, &MainWindow::changesCommitted);

   connect(rv->getRepoList(), &RepositoryView::clicked, this,
           qOverload<const QModelIndex &>(&MainWindow::onCommitClicked));
   connect(rv->getRepoList(), &RepositoryView::doubleClicked, this, &MainWindow::openCommitDiff);
   connect(rv->getRepoList(), &RepositoryView::signalAmmendCommit, this, &MainWindow::onAmmendCommit);
}

MainWindow::MainWindow(const QString &repo, QWidget *parent)
   : MainWindow(parent)
{
   QLog_Info("UI", "Initializing MainWindow with repo");

   setRepository(repo);
}

void MainWindow::updateUi()
{
   if (!mCurrentDir.isEmpty())
   {
      mGit->init(mCurrentDir, nullptr);

      ui->branchesWidget->showBranches();

      rv->clear(true);

      mGit->init2();

      const auto commitStackedIndex = ui->commitStackedWidget->currentIndex();
      const auto currentSha = commitStackedIndex == 0 ? ui->revisionWidget->getCurrentCommitSha() : QGit::ZERO_SHA;

      goToCommitSha(currentSha);

      if (commitStackedIndex == 1)
         ui->commitWidget->init(currentSha);
   }
}

void MainWindow::setRepository(const QString &newDir)
{
   if (!mRepositoryBusy && !newDir.isEmpty())
   {
      QLog_Info("UI", QString("Starting repository: %1").arg(newDir));

      mRepositoryBusy = true;

      const auto oldDir = mCurrentDir;

      resetWatcher(oldDir, newDir);

      bool archiveChanged;

      mGit->getBaseDir(newDir, mCurrentDir, archiveChanged);
      mGit->stop(archiveChanged);

      QLog_Info("UI", "Initializing Git...");

      const auto ok = mGit->init(mCurrentDir, nullptr); // blocking call

      if (ok)
      {
         QLog_Info("UI", "... Git initialized!");

         clearWindow(true);
         setWidgetsEnabled(true);

         mGit->init2();

         onCommitSelected(QGit::ZERO_SHA);
         ui->branchesWidget->showBranches();

         ui->commitWidget->init(QGit::ZERO_SHA);

         ui->mainStackedWidget->setCurrentIndex(0);
         ui->commitStackedWidget->setCurrentIndex(1);
         ui->controls->enableButtons(true);

         QLog_Info("UI", "MainWindow widgets configured with the new repo");
      }
      else
      {
         mCurrentDir = "";
         clearWindow(true);
         setWidgetsEnabled(false);
      }

      mRepositoryBusy = false;
   }
}

void MainWindow::resetWatcher(const QString &oldDir, const QString &newDir)
{
   if (!mGitWatcher)
   {
      mGitWatcher = new QFileSystemWatcher(this);
      connect(
          mGitWatcher, &QFileSystemWatcher::fileChanged, this,
          [this](const QString &path) {
             if (!path.endsWith(".autosave") and !path.endsWith(".tmp") and !path.endsWith(".user"))
                updateUi();
          },
          Qt::UniqueConnection);
   }

   if (oldDir != newDir)
   {
      QLog_Info("UI", QString("Resetting the file watcher from dir {%1} to {%2}").arg(oldDir, newDir));

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

void MainWindow::clearWindow(bool deepClear)
{
   blockSignals(true);

   ui->commitStackedWidget->setCurrentIndex(ui->commitStackedWidget->currentIndex());
   ui->mainStackedWidget->setCurrentIndex(0);

   ui->commitWidget->clear();
   ui->revisionWidget->clear();

   rv->clear(deepClear);
   ui->patchView->clear(deepClear);
   ui->fileDiffWidget->clear();
   ui->branchesWidget->clear();

   blockSignals(false);
}

void MainWindow::setWidgetsEnabled(bool enabled)
{
   ui->commitWidget->setEnabled(enabled);
   ui->revisionWidget->setEnabled(enabled);
   ui->commitStackedWidget->setEnabled(enabled);
   rv->setEnabled(enabled);
   ui->patchView->setEnabled(enabled);
   ui->fileDiffWidget->setEnabled(enabled);
   ui->branchesWidget->setEnabled(enabled);
}

void MainWindow::goToCommitSha(const QString &goToSha)
{
   QLog_Info("UI", QString("Setting the focus on the commit {%1}").arg(goToSha));

   const auto sha = mGit->getRefSha(goToSha);

   if (!sha.isEmpty())
   {
      rv->st.setSha(sha);
      rv->update(false, false);
   }
}

void MainWindow::openCommitDiff()
{
   ui->patchView->setStateInfo(rv->st);
   ui->mainStackedWidget->setCurrentIndex(1);
}

void MainWindow::changesCommitted(bool ok)
{
   if (ok)
      updateUi();
   else
      QMessageBox::critical(this, tr("Commit error"), tr("Failed to commit changes"));
}

void MainWindow::onCommitClicked(const QModelIndex &index)
{
   if (sender() == dynamic_cast<RepositoryView *>(rv->getRepoList()))
   {
      const auto shaIndex
          = rv->getRepoList()->model()->index(index.row(), static_cast<int>(RepositoryModel::FileHistoryColumn::SHA));
      const auto sha = shaIndex.data().toString();

      onCommitSelected(sha);
   }
}

void MainWindow::onCommitSelected(const QString &sha)
{
   const auto isWip = sha == QGit::ZERO_SHA;
   ui->commitStackedWidget->setCurrentIndex(isWip);

   QLog_Info("UI", QString("User selects the commit {%1}").arg(sha));

   if (isWip)
      ui->commitWidget->init(sha);
   else
      ui->revisionWidget->setCurrentCommitSha(sha);
}

void MainWindow::onAmmendCommit(const QString &sha)
{
   ui->commitStackedWidget->setCurrentIndex(1);
   ui->commitWidget->init(sha);
}

void MainWindow::onFileDiffRequested(const QString &currentSha, const QString &previousSha, const QString &file)
{
   const auto fileWithModifications = ui->fileDiffWidget->onFileDiffRequested(currentSha, previousSha, file);

   if (fileWithModifications)
   {
      QLog_Info(
          "UI",
          QString("Requested diff for file {%1} on between commits {%2} and {%3}").arg(file, currentSha, previousSha));

      ui->mainStackedWidget->setCurrentIndex(2);
   }
   else
      QMessageBox::information(this, tr("No modifications"), tr("There are no content modifications for this file"));
}

void MainWindow::rebase(const QString &from, const QString &to, const QString &onto)
{
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

   const auto success = from.isEmpty()
       ? mGit->run(QString("git checkout -q %1").arg(to)).first && mGit->run(QString("git rebase %1").arg(onto)).first
       : mGit->run(QString("git rebase --onto %3 %1^ %2").arg(from, to, onto)).first;

   if (success)
      updateUi();

   QApplication::restoreOverrideCursor();
}

void MainWindow::merge(const QStringList &shas, const QString &into)
{
   QString output;
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

   if (mGit->merge(into, shas, &output))
   {
      // TODO: Enable it again
      // controlsWidget->commitChanges();
      updateUi();
   }
   else if (!output.isEmpty())
      QMessageBox::warning(this, "git merge failed", QString("\n\nGit says: \n\n" + output));

   QApplication::restoreOverrideCursor();
}

void MainWindow::moveRef(const QString &target, const QString &toSHA)
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
         if ((children.count() == 0 || (children.count() == 1 && children.front() == QGit::ZERO_SHA)) && // no children
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

void MainWindow::closeEvent(QCloseEvent *ce)
{
   QLog_Info("UI", QString("Closing UI for repository {%1}").arg(mCurrentDir));

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
