#include "BranchContextMenu.h"

#include <BranchDlg.h>
#include <git.h>

#include <QApplication>
#include <QMessageBox>

BranchContextMenu::BranchContextMenu(const BranchContextMenuConfig &config, QWidget *parent)
   : QMenu(parent)
   , mConfig(config)
{
   if (mConfig.isLocal)
   {
      connect(addAction("Pull"), &QAction::triggered, this, &BranchContextMenu::pull);
      connect(addAction("Fetch"), &QAction::triggered, this, &BranchContextMenu::fetch);
   }

   if (mConfig.currentBranch == mConfig.branchSelected)
   {
      connect(addAction("Push"), &QAction::triggered, this, &BranchContextMenu::push);
      connect(addAction("Push force"), &QAction::triggered, this, &BranchContextMenu::pushForce);
   }

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
   QString output;
   const auto ret = Git::getInstance()->pull(output);
   QApplication::restoreOverrideCursor();

   if (ret)
      emit signalBranchesUpdated();
   else
      QMessageBox::critical(this, tr("Pull failed"), output);
}

void BranchContextMenu::fetch()
{
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   const auto ret = Git::getInstance()->fetch();
   QApplication::restoreOverrideCursor();

   if (ret)
      emit signalBranchesUpdated();
   else
      QMessageBox::critical(this, tr("Fetch failed"), tr("There were some problems while fetching. Please try again."));
}

void BranchContextMenu::push()
{
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   const auto ret = Git::getInstance()->push();
   QApplication::restoreOverrideCursor();

   if (ret)
      emit signalBranchesUpdated();
   else
      QMessageBox::critical(this, tr("Push failed"), tr("There were some problems while pushing. Please try again."));
}

void BranchContextMenu::pushForce()
{
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   const auto ret = Git::getInstance()->push(true);
   QApplication::restoreOverrideCursor();

   if (ret)
      emit signalBranchesUpdated();
   else
      QMessageBox::critical(this, tr("Push force failed"),
                            tr("There were some problems while pusing forced. Please try again."));
}

void BranchContextMenu::createBranch()
{
   BranchDlg dlg(mConfig.branchSelected, BranchDlgMode::CREATE);
   const auto ret = dlg.exec();

   if (ret == QDialog::Accepted)
      emit signalBranchesUpdated();
}

void BranchContextMenu::createCheckoutBranch()
{
   BranchDlg dlg(mConfig.branchSelected, BranchDlgMode::CREATE_CHECKOUT, this);
   const auto ret = dlg.exec();

   if (ret == QDialog::Accepted)
      emit signalBranchesUpdated();
}

void BranchContextMenu::merge()
{
   QString output;
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   const auto currentBranch = mConfig.currentBranch;
   const auto ret = Git::getInstance()->merge(currentBranch, { mConfig.branchSelected }, &output);
   QApplication::restoreOverrideCursor();

   if (ret)
      emit signalBranchesUpdated();
   else
      QMessageBox::critical(this, tr("Merge failed"),
                            tr("There were some problems during the merge. Please try again."));
}

void BranchContextMenu::rename()
{
   BranchDlg dlg(mConfig.branchSelected, BranchDlgMode::RENAME);
   const auto ret = dlg.exec();

   if (ret == QDialog::Accepted)
      emit signalBranchesUpdated();
}

void BranchContextMenu::deleteBranch()
{
   if (mConfig.branchSelected == "master")
      QMessageBox::critical(this, tr("Delete master?!"), tr("You are not allowed to delete master."), QMessageBox::Ok);
   else
   {
      auto ret = QMessageBox::warning(this, tr("Delete branch!"), tr("Are you sure you want to delete the branch?"),
                                      QMessageBox::Ok, QMessageBox::Cancel);

      if (ret == QMessageBox::Ok)
      {
         QByteArray output;
         QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

         const auto ret2 = mConfig.isLocal ? Git::getInstance()->removeLocalBranch(mConfig.branchSelected)
                                           : Git::getInstance()->removeRemoteBranch(mConfig.branchSelected);

         QApplication::restoreOverrideCursor();

         if (ret2.success)
            emit signalBranchesUpdated();
         else
            QMessageBox::critical(this, tr("Delete a branch failed"),
                                  tr("There were some problems while deleting the branch. Please try again."));
      }
   }
}
