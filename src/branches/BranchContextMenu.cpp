#include "BranchContextMenu.h"

#include <GitQlientStyles.h>
#include <BranchDlg.h>
#include <GitBranches.h>
#include <GitBase.h>
#include <GitRemote.h>

#include <QApplication>
#include <QMessageBox>

#include <utility>

BranchContextMenu::BranchContextMenu(BranchContextMenuConfig config, QWidget *parent)
   : QMenu(parent)
   , mConfig(std::move(config))
{
   setAttribute(Qt::WA_DeleteOnClose);

   if (mConfig.isLocal)
   {
      connect(addAction("Pull"), &QAction::triggered, this, &BranchContextMenu::pull);
      connect(addAction("Fetch"), &QAction::triggered, this, &BranchContextMenu::fetch);
      connect(addAction("Push"), &QAction::triggered, this, &BranchContextMenu::push);
   }

   if (mConfig.currentBranch == mConfig.branchSelected)
      connect(addAction("Push force"), &QAction::triggered, this, &BranchContextMenu::pushForce);

   addSeparator();

   connect(addAction("Create branch"), &QAction::triggered, this, &BranchContextMenu::createBranch);
   connect(addAction("Create && checkout branch"), &QAction::triggered, this, &BranchContextMenu::createCheckoutBranch);
   connect(addAction("Checkout branch"), &QAction::triggered, this, &BranchContextMenu::signalCheckoutBranch);

   if (mConfig.currentBranch != mConfig.branchSelected)
   {
      const auto actionName = QString("Merge %1 into %2").arg(mConfig.branchSelected, mConfig.currentBranch);
      connect(addAction(actionName), &QAction::triggered, this, &BranchContextMenu::merge);
   }

   addSeparator();

   connect(addAction("Rename"), &QAction::triggered, this, &BranchContextMenu::rename);
   connect(addAction("Delete"), &QAction::triggered, this, &BranchContextMenu::deleteBranch);
}

void BranchContextMenu::pull()
{
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   QScopedPointer<GitRemote> git(new GitRemote(mConfig.mGit));
   const auto ret = git->pull();
   QApplication::restoreOverrideCursor();

   if (ret.success)
      emit signalBranchesUpdated();
   else
   {
      const auto errorMsg = ret.output.toString();

      if (errorMsg.contains("error: could not apply", Qt::CaseInsensitive)
          && errorMsg.contains("causing a conflict", Qt::CaseInsensitive))
      {
         emit signalPullConflict();
      }
      else
      {
         QMessageBox msgBox(QMessageBox::Critical, tr("Error while pulling"),
                            QString("There were problems during the pull operation. Please, see the detailed "
                                    "description for more information."),
                            QMessageBox::Ok, this);
         msgBox.setDetailedText(errorMsg);
         msgBox.setStyleSheet(GitQlientStyles::getStyles());
         msgBox.exec();
      }
   }
}

void BranchContextMenu::fetch()
{
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   QScopedPointer<GitRemote> git(new GitRemote(mConfig.mGit));
   const auto ret = git->fetch();
   QApplication::restoreOverrideCursor();

   if (ret)
   {
      emit signalFetchPerformed();
      emit signalBranchesUpdated();
   }
   else
      QMessageBox::critical(this, tr("Fetch failed"), tr("There were some problems while fetching. Please try again."));
}

void BranchContextMenu::push()
{
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   QScopedPointer<GitRemote> git(new GitRemote(mConfig.mGit));
   const auto ret
       = mConfig.currentBranch == mConfig.branchSelected ? git->push() : git->pushBranch(mConfig.branchSelected);
   QApplication::restoreOverrideCursor();

   if (ret.output.toString().contains("has no upstream branch"))
   {
      BranchDlg dlg({ mConfig.branchSelected, BranchDlgMode::PUSH_UPSTREAM, mConfig.mGit });
      const auto ret = dlg.exec();

      if (ret == QDialog::Accepted)
         emit signalBranchesUpdated();
   }
   else if (ret.success)
      emit signalBranchesUpdated();
   else
   {
      QMessageBox msgBox(QMessageBox::Critical, tr("Error while pushing"),
                         QString("There were problems during the push operation. Please, see the detailed description "
                                 "for more information."),
                         QMessageBox::Ok, this);
      msgBox.setDetailedText(ret.output.toString());
      msgBox.setStyleSheet(GitQlientStyles::getStyles());
      msgBox.exec();
   }
}

void BranchContextMenu::pushForce()
{
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   QScopedPointer<GitRemote> git(new GitRemote(mConfig.mGit));
   const auto ret = git->push(true);
   QApplication::restoreOverrideCursor();

   if (ret.success)
   {
      emit signalRefreshPRsCache();
      emit signalBranchesUpdated();
   }
   else
   {
      QMessageBox msgBox(QMessageBox::Critical, tr("Error while pulling"),
                         QString("There were problems during the pull operation. Please, see the detailed description "
                                 "for more information."),
                         QMessageBox::Ok, this);
      msgBox.setDetailedText(ret.output.toString());
      msgBox.setStyleSheet(GitQlientStyles::getStyles());
      msgBox.exec();
   }
}

void BranchContextMenu::createBranch()
{
   BranchDlg dlg({ mConfig.branchSelected, BranchDlgMode::CREATE, mConfig.mGit });
   const auto ret = dlg.exec();

   if (ret == QDialog::Accepted)
      emit signalBranchesUpdated();
}

void BranchContextMenu::createCheckoutBranch()
{
   BranchDlg dlg({ mConfig.branchSelected, BranchDlgMode::CREATE_CHECKOUT, mConfig.mGit });
   const auto ret = dlg.exec();

   if (ret == QDialog::Accepted)
      emit signalBranchesUpdated();
}

void BranchContextMenu::merge()
{
   emit signalMergeRequired(mConfig.currentBranch, mConfig.branchSelected);
}

void BranchContextMenu::rename()
{
   BranchDlg dlg({ mConfig.branchSelected, BranchDlgMode::RENAME, mConfig.mGit });
   const auto ret = dlg.exec();

   if (ret == QDialog::Accepted)
      emit signalBranchesUpdated();
}

void BranchContextMenu::deleteBranch()
{
   if (!mConfig.isLocal && mConfig.branchSelected == "master")
      QMessageBox::critical(this, tr("Delete master?!"), tr("You are not allowed to delete remote master."),
                            QMessageBox::Ok);
   else
   {
      auto ret = QMessageBox::warning(this, tr("Delete branch!"), tr("Are you sure you want to delete the branch?"),
                                      QMessageBox::Ok, QMessageBox::Cancel);

      if (ret == QMessageBox::Ok)
      {
         QScopedPointer<GitBranches> git(new GitBranches(mConfig.mGit));
         QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
         const auto ret2 = mConfig.isLocal ? git->removeLocalBranch(mConfig.branchSelected)
                                           : git->removeRemoteBranch(mConfig.branchSelected);
         QApplication::restoreOverrideCursor();

         if (ret2.success)
            emit signalBranchesUpdated();
         else
            QMessageBox::critical(
                this, tr("Delete a branch failed"),
                tr("There were some problems while deleting the branch:<br><br> %1").arg(ret2.output.toString()));
      }
   }
}
