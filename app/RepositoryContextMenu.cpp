#include "RepositoryContextMenu.h"

#include <git.h>
#include <WorkInProgressWidget.h>
#include <BranchDlg.h>
#include <TagDlg.h>

#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>

#include <QLogger.h>

using namespace QLogger;

RepositoryContextMenu::RepositoryContextMenu(QSharedPointer<Git> git, const QStringList &shas, QWidget *parent)
   : QMenu(parent)
   , mGit(git)
   , mShas(shas)
{
   if (shas.count() == 1)
      createIndividualShaMenu();
   else
      createMultipleShasMenu();
}

void RepositoryContextMenu::createIndividualShaMenu()
{
   if (mShas.count() == 1)
   {
      const auto sha = mShas.first();

      if (sha == ZERO_SHA)
      {
         const auto stashAction = addAction("Push stash");
         connect(stashAction, &QAction::triggered, this, &RepositoryContextMenu::stashPush);

         const auto popAction = addAction("Pop stash");
         connect(popAction, &QAction::triggered, this, &RepositoryContextMenu::stashPop);

         const auto commitAction = addAction("Commit");
         // connect(commitAction, &QAction::triggered, this, [this]() {});
         addAction(commitAction);
      }

      const auto commitAction = addAction("See diff");
      connect(commitAction, &QAction::triggered, this, [this]() { emit signalOpenDiff(mShas.first()); });

      if (sha != ZERO_SHA)
      {
         const auto createBranchAction = addAction("Create branch here");
         connect(createBranchAction, &QAction::triggered, this, &RepositoryContextMenu::createBranch);

         const auto createTagAction = addAction("Create tag here");
         connect(createTagAction, &QAction::triggered, this, &RepositoryContextMenu::createTag);

         const auto exportAsPatchAction = addAction("Export as patch");
         connect(exportAsPatchAction, &QAction::triggered, this, &RepositoryContextMenu::exportAsPatch);

         addSeparator();

         const auto checkoutCommitAction = addAction("Checkout commit");
         connect(checkoutCommitAction, &QAction::triggered, this, &RepositoryContextMenu::checkoutCommit);

         QByteArray output;
         auto ret = mGit->getBranchesOfCommit(sha);
         const auto currentBranch = mGit->getCurrentBranchName();

         if (ret.success)
         {
            auto branches = ret.output.toString().split('\n');

            for (auto &branch : branches)
            {
               if (branch.contains("*"))
                  branch.remove("*");

               if (branch.contains("->"))
               {
                  branch.clear();
                  continue;
               }

               branch.remove("remotes/");
               branch = branch.trimmed();

               if (!branch.isEmpty() && branch != currentBranch && branch != QString("origin/%1").arg(currentBranch))
               {
                  const auto checkoutCommitAction = addAction(QString(tr("Checkout %1")).arg(branch));
                  checkoutCommitAction->setDisabled(true);
                  // connect(checkoutCommitAction, &QAction::triggered, this, &RepositoryView::executeAction);
               }
            }

            for (auto branch : qAsConst(branches))
            {
               if (!branch.isEmpty() && branch != currentBranch && branch != QString("origin/%1").arg(currentBranch))
               {
                  // If is the last commit of a branch
                  const auto mergeBranchAction = addAction(QString(tr("Merge %1")).arg(branch));
                  connect(mergeBranchAction, &QAction::triggered, this, [this, branch]() { merge(branch); });
               }
            }

            addSeparator();

            auto isCommitInCurrentBranch = false;

            for (auto branch : qAsConst(branches))
               isCommitInCurrentBranch |= branch == currentBranch;

            if (!isCommitInCurrentBranch)
            {
               const auto cherryPickAction = addAction(tr("Cherry pick commit"));
               connect(cherryPickAction, &QAction::triggered, this, &RepositoryContextMenu::cherryPickCommit);
            }
         }

         ret = mGit->getLastCommitOfBranch(currentBranch);

         if (ret.success)
         {
            const auto lastShaStr = ret.output.toString().remove('\n');

            if (lastShaStr == sha)
            {
               const auto amendCommitAction = addAction("Amend");
               connect(amendCommitAction, &QAction::triggered, this,
                       [this]() { emit signalAmendCommit(mShas.first()); });

               const auto applyPatchAction = addAction("Apply patch");
               connect(applyPatchAction, &QAction::triggered, this, &RepositoryContextMenu::applyPatch);

               const auto applyCommitAction = addAction("Apply commit");
               connect(applyCommitAction, &QAction::triggered, this, &RepositoryContextMenu::applyCommit);

               const auto pushAction = addAction("Push");
               connect(pushAction, &QAction::triggered, this, &RepositoryContextMenu::push);

               const auto pullAction = addAction("Pull");
               connect(pullAction, &QAction::triggered, this, &RepositoryContextMenu::pull);

               const auto fetchAction = addAction("Fetch");
               connect(fetchAction, &QAction::triggered, this, &RepositoryContextMenu::fetch);
            }
         }

         const auto copyShaAction = addAction("Copy SHA");
         connect(copyShaAction, &QAction::triggered, this,
                 [this]() { QApplication::clipboard()->setText(mShas.first()); });

         addSeparator();

         const auto resetSoftAction = addAction("Reset - Soft");
         connect(resetSoftAction, &QAction::triggered, this, &RepositoryContextMenu::resetSoft);

         const auto resetMixedAction = addAction("Reset - Mixed");
         connect(resetMixedAction, &QAction::triggered, this, &RepositoryContextMenu::resetMixed);

         const auto resetHardAction = addAction("Reset - Hard");
         connect(resetHardAction, &QAction::triggered, this, &RepositoryContextMenu::resetHard);
      }
   }
}

void RepositoryContextMenu::createMultipleShasMenu()
{
   if (mShas.count() == 2)
   {
      const auto diffAction = addAction("See diff");
      connect(diffAction, &QAction::triggered, this, [this]() { emit signalOpenCompareDiff(mShas); });
   }

   if (!mShas.contains(ZERO_SHA))
   {
      const auto exportAsPatchAction = addAction("Export as patch");
      connect(exportAsPatchAction, &QAction::triggered, this, &RepositoryContextMenu::exportAsPatch);

      const auto copyShaAction = addAction("Copy all SHA");
      connect(copyShaAction, &QAction::triggered, this,
              [this]() { QApplication::clipboard()->setText(mShas.join(',')); });
   }
   else
      QLog_Warning("UI", "WIP selected as part of a series of SHAs");
}

void RepositoryContextMenu::stashPush()
{
   const auto ret = mGit->stash();

   if (ret)
      emit signalRepositoryUpdated();
}

void RepositoryContextMenu::stashPop()
{
   const auto ret = mGit->pop();

   if (ret.success)
      emit signalRepositoryUpdated();
}

void RepositoryContextMenu::createBranch()
{
   BranchDlg dlg({ mShas.first(), BranchDlgMode::CREATE_FROM_COMMIT, QSharedPointer<Git>(mGit) });
   const auto ret = dlg.exec();

   if (ret == QDialog::Accepted)
      emit signalRepositoryUpdated();
}

void RepositoryContextMenu::createTag()
{
   TagDlg dlg(mGit, mShas.first());
   const auto ret = dlg.exec();

   if (ret == QDialog::Accepted)
      emit signalRepositoryUpdated();
}

#include <QProcess>
void RepositoryContextMenu::exportAsPatch()
{
   const auto ret = mGit->exportPatch(mShas);

   if (ret.success)
   {
      const auto action = QMessageBox::information(
          this, tr("Patch generated"),
          tr("<p>The patch has been generated!</p>"
             "<p><b>Commit:</b></p><p>%1</p>"
             "<p><b>Destination:</b> %2</p>"
             "<p><b>File names:</b></p><p>%3</p>")
              .arg(mShas.join("<br>"), mGit->getWorkingDir(), ret.output.toStringList().join("<br>")),
          QMessageBox::Ok, QMessageBox::Open);

      if (action == QMessageBox::Open)
      {
         QProcess proc;
         QString fileBrowser;

#ifdef Q_OS_LINUX
         fileBrowser.append("xdg-open");
#elif defined(Q_OS_WIN)
         fileBrowser.append("explorer.exe");
#endif

         proc.startDetached(QString("%1 %2").arg(fileBrowser, mGit->getWorkingDir()));
      }
   }
}

void RepositoryContextMenu::checkoutCommit()
{
   const auto sha = mShas.first();
   QLog_Info("UI", QString("Checking out the commit {%1}").arg(sha));

   const auto ret = mGit->checkoutCommit(sha);

   if (ret.success)
      emit signalRepositoryUpdated();
   else
      QMessageBox::critical(this, tr("Checkout error"), ret.output.toString());
}

void RepositoryContextMenu::cherryPickCommit()
{
   const auto ret = mGit->cherryPickCommit(mShas.first());

   if (ret.success)
      emit signalRepositoryUpdated();
   else
      QMessageBox::critical(this, tr("Error while cherry-pick"), ret.output.toString());
}

void RepositoryContextMenu::applyPatch()
{
   const QString fileName(QFileDialog::getOpenFileName(this, "Select a patch to apply"));

   if (!fileName.isEmpty() && mGit->apply(fileName))
      emit signalRepositoryUpdated();
}

void RepositoryContextMenu::applyCommit()
{
   const QString fileName(QFileDialog::getOpenFileName(this, "Select a patch to apply"));

   if (!fileName.isEmpty() && mGit->apply(fileName, true))
      emit signalRepositoryUpdated();
}

void RepositoryContextMenu::push()
{
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   const auto ret = mGit->push();
   QApplication::restoreOverrideCursor();

   if (ret.success)
      emit signalRepositoryUpdated();
   else
      QMessageBox::critical(this, tr("Error while pushin"), ret.output.toString());
}

void RepositoryContextMenu::pull()
{
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   QString output;
   const auto ret = mGit->pull();
   QApplication::restoreOverrideCursor();

   if (ret.success)
      emit signalRepositoryUpdated();
   else
      QMessageBox::critical(this, tr("Error while pulling"), ret.output.toString());
}

void RepositoryContextMenu::fetch()
{
   if (mGit->fetch())
      emit signalRepositoryUpdated();
}

void RepositoryContextMenu::resetSoft()
{
   if (mGit->resetCommit(mShas.first(), Git::CommitResetType::SOFT))
      emit signalRepositoryUpdated();
}

void RepositoryContextMenu::resetMixed()
{
   if (mGit->resetCommit(mShas.first(), Git::CommitResetType::MIXED))
      emit signalRepositoryUpdated();
}

void RepositoryContextMenu::resetHard()
{
   const auto retMsg = QMessageBox::warning(
       this, "Reset hard requested!", "Are you sure you want to reset the branch to this commit in a <b>hard</b> way?",
       QMessageBox::Ok, QMessageBox::Cancel);

   if (retMsg == QMessageBox::Ok)
   {
      if (mGit->resetCommit(mShas.first(), Git::CommitResetType::HARD))
         emit signalRepositoryUpdated();
   }
}

void RepositoryContextMenu::merge(const QString &branchFrom)
{
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   const auto currentBranch = mGit->getCurrentBranchName();
   const auto ret = mGit->merge(currentBranch, { branchFrom });
   QApplication::restoreOverrideCursor();

   const auto outputStr = ret.output.toString();

   if (ret.success)
   {
      emit signalRepositoryUpdated();

      if (!outputStr.isEmpty())
         QMessageBox::information(parentWidget(), tr("Merge status"), outputStr);
   }
   else
      QMessageBox::critical(parentWidget(), tr("Merge failed"), outputStr);
}
