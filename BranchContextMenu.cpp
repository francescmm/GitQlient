#include "BranchContextMenu.h"

#include <BranchDlg.h>
#include <git.h>

#include <QApplication>
#include <QMessageBox>

BranchContextMenu::BranchContextMenu(const QString &branchSelected, bool isLocal, QWidget *parent)
   : QMenu(parent)
   , mSelectedBranch(branchSelected)
   , mLocal(isLocal)
{
   const auto currentBranch = Git::getInstance()->getCurrentBranchName();

   if (mLocal)
   {
      const auto pullAction = new QAction("Pull");
      connect(pullAction, &QAction::triggered, this, &BranchContextMenu::pull);
      addAction(pullAction);

      const auto fetchAction = new QAction("Fetch");
      connect(fetchAction, &QAction::triggered, this, &BranchContextMenu::fetch);
      addAction(fetchAction);
   }

   if (currentBranch == branchSelected)
   {

      const auto pushAction = new QAction("Push");
      connect(pushAction, &QAction::triggered, this, &BranchContextMenu::push);
      addAction(pushAction);

      const auto pushForceAction = new QAction("Push force");
      connect(pushForceAction, &QAction::triggered, this, &BranchContextMenu::pushForce);
      addAction(pushForceAction);
   }

   addSeparator();

   const auto createBranchAction = new QAction("Create branch");
   connect(createBranchAction, &QAction::triggered, this, &BranchContextMenu::createBranch);
   addAction(createBranchAction);

   const auto createCheckoutBranchAction = new QAction("Create and checkout branch");
   connect(createCheckoutBranchAction, &QAction::triggered, this, &BranchContextMenu::createAndCheckoutBranch);
   addAction(createCheckoutBranchAction);
   addSeparator();

   const auto checkoutBranchAction = new QAction("Checkout branch");
   connect(checkoutBranchAction, &QAction::triggered, this, &BranchContextMenu::signalCheckoutBranch);
   addAction(checkoutBranchAction);

   if (currentBranch != branchSelected)
   {
      const auto actionName = QString("Merge %1 into %2").arg(branchSelected, currentBranch);
      const auto mergeMasterAction = new QAction(actionName);

      connect(mergeMasterAction, &QAction::triggered, this, &BranchContextMenu::merge);
      addAction(mergeMasterAction);
   }

   const auto renameBranchAction = new QAction("Rename");
   connect(renameBranchAction, &QAction::triggered, this, &BranchContextMenu::rename);
   addAction(renameBranchAction);

   const auto deleteBranchAction = new QAction("Delete");
   connect(deleteBranchAction, &QAction::triggered, this, &BranchContextMenu::deleteBranch);
   addAction(deleteBranchAction);
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
      QMessageBox::critical(this, "Pull failed", output);
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
   BranchDlg dlg(mSelectedBranch, BranchDlgMode::CREATE);
   const auto ret = dlg.exec();

   if (ret == QDialog::Accepted)
      emit signalBranchesUpdated();
}

void BranchContextMenu::createAndCheckoutBranch()
{
   BranchDlg dlg(mSelectedBranch, BranchDlgMode::CREATE_CHECKOUT);
   const auto ret = dlg.exec();

   if (ret == QDialog::Accepted)
      emit signalBranchesUpdated();
}

void BranchContextMenu::merge()
{
   QString output;
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   const auto currentBranch = Git::getInstance()->getCurrentBranchName();
   const auto ret = Git::getInstance()->merge(currentBranch, { mSelectedBranch }, &output);
   QApplication::restoreOverrideCursor();

   if (ret)
      emit signalBranchesUpdated();
   else
      QMessageBox::critical(this, tr("Merge failed"),
                            tr("There were some problems during the merge. Please try again."));
}

void BranchContextMenu::rename()
{
   BranchDlg dlg(mSelectedBranch, BranchDlgMode::RENAME);
   const auto ret = dlg.exec();

   if (ret == QDialog::Accepted)
      emit signalBranchesUpdated();
}

void BranchContextMenu::deleteBranch()
{
   if (mSelectedBranch == "master")
      QMessageBox::critical(this, "Master delete!", "You are not allowed to delete master.", QMessageBox::Ok);
   else
   {
      auto ret = QMessageBox::warning(this, "Branch delete!", "Are you sure you want to delete the branch?",
                                      QMessageBox::Ok, QMessageBox::Cancel);

      if (ret == QMessageBox::Ok)
      {
         QByteArray output;
         QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

         ret = mLocal ? Git::getInstance()->removeLocalBranch(mSelectedBranch, output)
                      : Git::getInstance()->removeRemoteBranch(mSelectedBranch, output);

         QApplication::restoreOverrideCursor();

         if (ret)
            emit signalBranchesUpdated();
         else
            QMessageBox::critical(this, tr("Delete a branch failed"),
                                  tr("There were some problems while deleting the branch. Please try again."));
      }
   }
}
