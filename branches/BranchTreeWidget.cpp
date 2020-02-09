#include "BranchTreeWidget.h"

#include <GitBranches.h>
#include <GitBase.h>
#include <BranchContextMenu.h>

#include <QApplication>
#include <QMessageBox>

BranchTreeWidget::BranchTreeWidget(const QSharedPointer<GitBase> &git, QWidget *parent)
   : QTreeWidget(parent)
   , mGit(git)
{
   setContextMenuPolicy(Qt::CustomContextMenu);

   connect(this, &BranchTreeWidget::customContextMenuRequested, this, &BranchTreeWidget::showBranchesContextMenu);
   connect(this, &BranchTreeWidget::itemClicked, this, &BranchTreeWidget::selectCommit);
   connect(this, &BranchTreeWidget::itemDoubleClicked, this, &BranchTreeWidget::checkoutBranch);
}

void BranchTreeWidget::showBranchesContextMenu(const QPoint &pos)
{
   const auto item = itemAt(pos);

   if (item && item->data(0, Qt::UserRole + 2).toBool())
   {
      auto currentBranch = mGit->getCurrentBranch();
      auto selectedBranch = item->data(0, Qt::UserRole + 1).toString();

      if (!mLocal)
         selectedBranch = selectedBranch.remove("origin/");

      const auto menu = new BranchContextMenu({ currentBranch, selectedBranch, mLocal, mGit }, this);
      connect(menu, &BranchContextMenu::signalBranchesUpdated, this, &BranchTreeWidget::signalBranchesUpdated);
      connect(menu, &BranchContextMenu::signalCheckoutBranch, this, [this, item]() { checkoutBranch(item); });

      menu->exec(viewport()->mapToGlobal(pos));
   }
}

void BranchTreeWidget::checkoutBranch(QTreeWidgetItem *item)
{
   if (item->data(0, Qt::UserRole + 2).toBool())
   {
      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
      QScopedPointer<GitBranches> git(new GitBranches(mGit));
      const auto ret = git->checkoutRemoteBranch(item->data(0, Qt::UserRole + 1).toString().remove("origin/"));
      QApplication::restoreOverrideCursor();

      if (ret.success)
         emit signalBranchCheckedOut();
      else
         QMessageBox::critical(this, tr("Checkout branch error"), ret.output.toString());
   }
}

void BranchTreeWidget::selectCommit(QTreeWidgetItem *item)
{
   if (item->data(0, Qt::UserRole + 2).toBool())
   {
      const auto branchName = item->data(0, Qt::UserRole + 1).toString();
      QScopedPointer<GitBranches> git(new GitBranches(mGit));
      const auto ret = git->getLastCommitOfBranch(branchName);

      emit signalSelectCommit(ret.output.toString());
   }
}
