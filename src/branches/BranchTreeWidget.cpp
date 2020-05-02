#include "BranchTreeWidget.h"

#include <GitBranches.h>
#include <GitBase.h>
#include <BranchContextMenu.h>
#include <PullDlg.h>

#include <QApplication>
#include <QMessageBox>

BranchTreeWidget::BranchTreeWidget(const QSharedPointer<GitBase> &git, QWidget *parent)
   : QTreeWidget(parent)
   , mGit(git)
{
   setContextMenuPolicy(Qt::CustomContextMenu);
   setAttribute(Qt::WA_DeleteOnClose);

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
      connect(menu, &BranchContextMenu::signalMergeRequired, this, &BranchTreeWidget::signalMergeRequired);
      connect(menu, &BranchContextMenu::signalPullConflict, this, &BranchTreeWidget::signalPullConflict);

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

      const auto output = ret.output.toString();

      if (ret.success)
      {
         QRegExp rx("by \\d+ commits");
         rx.indexIn(output);
         auto value = rx.capturedTexts().constFirst().split(" ");
         auto uiUpdateRequested = false;

         if (value.count() == 3 && output.contains("your branch is behind", Qt::CaseInsensitive))
         {
            const auto commits = value.at(1).toUInt();
            (void)commits;

            PullDlg pull(mGit, output.split('\n').first());
            connect(&pull, &PullDlg::signalRepositoryUpdated, this, &BranchTreeWidget::signalBranchCheckedOut);
            connect(&pull, &PullDlg::signalPullConflict, this, &BranchTreeWidget::signalPullConflict);

            if (pull.exec() == QDialog::Accepted)
               uiUpdateRequested = true;
         }

         if (!uiUpdateRequested)
            emit signalBranchCheckedOut();
      }
      else
         QMessageBox::critical(this, tr("Checkout branch error"), output);
   }
}

void BranchTreeWidget::selectCommit(QTreeWidgetItem *item)
{
   if (item->data(0, Qt::UserRole + 2).toBool())
      emit signalSelectCommit(item->data(0, Qt::UserRole + 3).toString());
}
