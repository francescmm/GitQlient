#include "MainWindow.h"
#include <ui_MainWindow.h>

#include <RepositoryModel.h>
#include <RepositoryView.h>
#include <git.h>
#include <revsview.h>

#include <QDirIterator>
#include <QFileSystemWatcher>
#include <QFileDialog>
#include <QMessageBox>

using namespace QGit;

QTextBrowser *QGit::kErrorLogBrowser = nullptr;

MainWindow::MainWindow(QWidget *p)
   : QFrame(p)
   , ui(new Ui::MainWindow)
   , rv(new RevsView(true))
{
   QFile styles(":/stylesheet.css");

   if (styles.open(QIODevice::ReadOnly))
   {
      setStyleSheet(QString::fromUtf8(styles.readAll()));
      styles.close();
   }

   ui->setupUi(this);

   setObjectName("mainWindow");
   setWindowTitle("Heimdall");

   ui->mainStackedWidget->setCurrentIndex(0);
   ui->page_5->layout()->addWidget(rv->getRepoList());

   Git::getInstance(this);

   QGit::kErrorLogBrowser = ui->outputTerminal;

   ui->revisionWidget->setup(rv);

   qApp->installEventFilter(this);

   rv->setEnabled(false);

   connect(ui->controls, &Controls::signalOpenRepo, this, &MainWindow::setRepository);
   connect(ui->controls, &Controls::signalGoBack, this, [this]() { ui->mainStackedWidget->setCurrentIndex(0); });
   connect(ui->controls, &Controls::signalRepositoryUpdated, this, &MainWindow::updateUi);
   connect(ui->controls, &Controls::signalGoToSha, this, &MainWindow::goToCommitSha);

   connect(ui->leGitCommand, &QLineEdit::returnPressed, this, &MainWindow::executeCommand);

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
   connect(rv->getRepoList(), &RepositoryView::showStatusMessage, ui->outputTerminal, &QTextBrowser::setText);
   connect(rv->getRepoList(), &RepositoryView::signalAmmendCommit, this, &MainWindow::onAmmendCommit);
}

void MainWindow::updateUi()
{
   Git::getInstance()->init(mCurrentDir, nullptr);

   ui->branchesWidget->showBranches();

   rv->clear(true);
   Git::getInstance()->init2();

   auto currentSha = QGit::ZERO_SHA;

   if (ui->commitStackedWidget->currentIndex() == 0)
      currentSha = ui->revisionWidget->getCurrentCommitSha();

   goToCommitSha(currentSha);

   if (ui->commitStackedWidget->currentIndex() == 1)
      ui->commitWidget->init(currentSha);
}

void MainWindow::setRepository(const QString &newDir)
{
   if (!mRepositoryBusy)
   {
      mRepositoryBusy = true;

      const auto oldDir = mCurrentDir;

      resetWatcher(oldDir, newDir);

      bool archiveChanged;
      const auto git = Git::getInstance();
      git->getBaseDir(newDir, mCurrentDir, archiveChanged);
      git->stop(archiveChanged); // stop all pending processes, non blocking

      bool ok = git->init(mCurrentDir, nullptr); // blocking call

      if (ok)
      {
         clearWindow(true);
         setWidgetsEnabled(true);

         Git::getInstance()->init2();

         onCommitSelected(QGit::ZERO_SHA);
         ui->branchesWidget->showBranches();

         ui->commitWidget->init(QGit::ZERO_SHA);

         ui->mainStackedWidget->setCurrentIndex(0);
         ui->commitStackedWidget->setCurrentIndex(1);
      }
      else
      {
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

   ui->commitStackedWidget->setCurrentIndex(1);
   ui->mainStackedWidget->setCurrentIndex(0);

   ui->commitWidget->clear();
   ui->revisionWidget->clear();
   ui->leGitCommand->clear();
   ui->outputTerminal->clearHistory();
   ui->outputTerminal->clear();
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
   ui->outputTerminal->setEnabled(enabled);
   rv->setEnabled(enabled);
   ui->patchView->setEnabled(enabled);
   ui->fileDiffWidget->setEnabled(enabled);
   ui->branchesWidget->setEnabled(enabled);
}

void MainWindow::executeCommand()
{
   if (!ui->leGitCommand->text().isEmpty())
   {
      if (ui->leGitCommand->text() == "clear")
         ui->outputTerminal->clear();
      else if (ui->leGitCommand->text() == "ui update")
         updateUi();
      else
      {
         QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
         QString output;
         Git::getInstance()->run(ui->leGitCommand->text(), &output);
         QApplication::restoreOverrideCursor();
         ui->outputTerminal->setText(output);
      }
   }
   ui->leGitCommand->setText("");
}

void MainWindow::goToCommitSha(const QString &goToSha)
{
   const auto sha = Git::getInstance()->getRefSha(goToSha);

   if (!sha.isEmpty())
   {
      rv->st.setSha(sha);
      rv->update(false, false);
   }
   else
      rv->getRepoList()->scrollToNext(0);
}

void MainWindow::openCommitDiff()
{
   ui->patchView->setStateInfo(rv->st);
   ui->mainStackedWidget->setCurrentIndex(1);
}

void MainWindow::changesCommitted(bool ok)
{
   ok ? updateUi() : ui->outputTerminal->setText("Failed to commit changes");
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

   if (isWip)
      ui->commitWidget->init(sha);
   else
      ui->revisionWidget->setCurrentCommitSha(sha);
}

void MainWindow::onAmmendCommit(const QString &sha)
{
   ui->commitStackedWidget->setCurrentIndex(1);
   // ui->revisionWidget->setCurrentCommitSha(sha);
   ui->commitWidget->init(sha);
}

void MainWindow::onFileDiffRequested(const QString &currentSha, const QString &previousSha, const QString &file)
{
   if (ui->fileDiffWidget->onFileDiffRequested(currentSha, previousSha, file))
      ui->mainStackedWidget->setCurrentIndex(2);
   else
      QMessageBox::information(this, tr("No modifications"), tr("There are no content modifications for this file"));
}

void MainWindow::rebase(const QString &from, const QString &to, const QString &onto)
{
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

   const auto git = Git::getInstance();
   const auto success = from.isEmpty()
       ? git->run(QString("git checkout -q %1").arg(to)) && git->run(QString("git rebase %1").arg(onto))
       : git->run(QString("git rebase --onto %3 %1^ %2").arg(from, to, onto));

   if (success)
      updateUi();

   QApplication::restoreOverrideCursor();
}

void MainWindow::merge(const QStringList &shas, const QString &into)
{
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   QString output;
   if (Git::getInstance()->merge(into, shas, &output))
   {

      // TODO: Enable it again
      // controlsWidget->commitChanges();

      ui->outputTerminal->setText(QString("Successfully merged into %1").arg(into));

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
      const auto git = Git::getInstance();
      const QString &sha = git->getRefSha(target, Git::BRANCH, false);
      if (!sha.isEmpty())
      {
         const QStringList &children = git->getChildren(sha);
         if ((children.count() == 0 || (children.count() == 1 && children.front() == ZERO_SHA)) && // no children
             git->getRefNames(sha, Git::ANY_REF).count() == 1 && // last ref name
             QMessageBox::question(this, "move branch",
                                   QString("This is the last reference to this branch.\n"
                                           "Do you really want to move '%1'?")
                                       .arg(target))
                 == QMessageBox::No)
            return;

         if (target == git->getCurrentBranchName()) // move current branch
            cmd = QString("git checkout -q -B %1 %2").arg(target, toSHA);
         else // move any other local branch
            cmd = QString("git branch -f %1 %2").arg(target, toSHA);
      }
   }

   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

   if (Git::getInstance()->run(cmd))
      updateUi();

   QApplication::restoreOverrideCursor();
}

void MainWindow::closeEvent(QCloseEvent *ce)
{
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

   Git::getInstance()->stop(true);

   QWidget::closeEvent(ce);
}
