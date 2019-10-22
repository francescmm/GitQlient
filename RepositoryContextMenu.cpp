#include "RepositoryContextMenu.h"

#include <git.h>
#include <CommitWidget.h>
#include <BranchDlg.h>
#include <TagDlg.h>

#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>

RepositoryContextMenu::RepositoryContextMenu(QSharedPointer<Git> git, const QString &sha, QWidget *parent)
   : QMenu(parent)
   , mGit(git)
   , mSha(sha)
{
   if (mSha == ZERO_SHA)
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
   connect(commitAction, &QAction::triggered, this, [this]() { emit signalOpenDiff(mSha); });

   if (mSha != ZERO_SHA)
   {
      const auto amendCommitAction = addAction("Amend");
      connect(amendCommitAction, &QAction::triggered, this, &RepositoryContextMenu::signalAmendCommit);

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
      auto ret = mGit->getBranchesOfCommit(mSha);
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
               mergeBranchAction->setDisabled(true);
               // connect(mergeBranchAction, &QAction::triggered, this, &RepositoryView::executeAction);
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

         if (lastShaStr == mSha)
         {
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
      connect(copyShaAction, &QAction::triggered, this, [this]() { QApplication::clipboard()->setText(mSha); });

      addSeparator();

      const auto resetSoftAction = addAction("Reset - Soft");
      connect(resetSoftAction, &QAction::triggered, this, &RepositoryContextMenu::resetSoft);

      const auto resetMixedAction = addAction("Reset - Mixed");
      connect(resetMixedAction, &QAction::triggered, this, &RepositoryContextMenu::resetMixed);

      const auto resetHardAction = addAction("Reset - Hard");
      connect(resetHardAction, &QAction::triggered, this, &RepositoryContextMenu::resetHard);
   }
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

   if (ret)
      emit signalRepositoryUpdated();
}

void RepositoryContextMenu::createBranch()
{
   BranchDlg dlg({ mSha, BranchDlgMode::CREATE_FROM_COMMIT, QSharedPointer<Git>(mGit) });
   const auto ret = dlg.exec();

   if (ret == QDialog::Accepted)
      emit signalRepositoryUpdated();
}

void RepositoryContextMenu::createTag()
{
   TagDlg dlg(mGit, mSha);
   const auto ret = dlg.exec();

   if (ret == QDialog::Accepted)
      emit signalRepositoryUpdated();
}

void RepositoryContextMenu::checkoutCommit()
{
   const auto ret = mGit->checkoutCommit(mSha);

   if (ret.success)
      emit signalRepositoryUpdated();
}

void RepositoryContextMenu::cherryPickCommit()
{
   if (mGit->cherryPickCommit(mSha))
      emit signalRepositoryUpdated();
}

void RepositoryContextMenu::applyPatch()
{
   const QString fileName(QFileDialog::getOpenFileName(this, "Select a patch to apply"));

   if (!fileName.isEmpty())
      mGit->apply(fileName);
}

void RepositoryContextMenu::applyCommit()
{
   const QString dirName(QFileDialog::getExistingDirectory(this, "Choose the file to apply"));
}

void RepositoryContextMenu::push()
{
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   const auto ret = mGit->push();
   QApplication::restoreOverrideCursor();

   if (ret)
      emit signalRepositoryUpdated();
}

void RepositoryContextMenu::pull()
{
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   QString output;
   const auto ret = mGit->pull(output);
   QApplication::restoreOverrideCursor();

   if (ret)
      emit signalRepositoryUpdated();
}

void RepositoryContextMenu::fetch()
{
   if (mGit->fetch())
      emit signalRepositoryUpdated();
}

void RepositoryContextMenu::resetSoft()
{
   if (mGit->resetCommit(mSha, Git::CommitResetType::SOFT))
      emit signalRepositoryUpdated();
}

void RepositoryContextMenu::resetMixed()
{
   if (mGit->resetCommit(mSha, Git::CommitResetType::MIXED))
      emit signalRepositoryUpdated();
}

void RepositoryContextMenu::resetHard()
{
   const auto retMsg = QMessageBox::warning(
       this, "Reset hard requested!", "Are you sure you want to reset the branch to this commit in a <b>hard</b> way?",
       QMessageBox::Ok, QMessageBox::Cancel);

   if (retMsg == QMessageBox::Ok)
   {
      if (mGit->resetCommit(mSha, Git::CommitResetType::HARD))
         emit signalRepositoryUpdated();
   }
}
