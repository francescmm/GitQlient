#include "BranchTreeWidget.h"

#include <GitQlientStyles.h>
#include <GitBranches.h>
#include <GitBase.h>
#include <BranchContextMenu.h>
#include <PullDlg.h>
#include <GitQlientBranchItemRole.h>
#include <AddRemoteDlg.h>
#include <GitRemote.h>

#include <QApplication>
#include <QMessageBox>

using namespace GitQlient;

BranchTreeWidget::BranchTreeWidget(const QSharedPointer<GitBase> &git, QWidget *parent)
   : QTreeWidget(parent)
   , mGit(git)
{
   setContextMenuPolicy(Qt::CustomContextMenu);
   setAttribute(Qt::WA_DeleteOnClose);

   connect(this, &BranchTreeWidget::customContextMenuRequested, this, &BranchTreeWidget::showBranchesContextMenu);
   connect(this, &BranchTreeWidget::itemClicked, this, &BranchTreeWidget::selectCommit);
   connect(this, &BranchTreeWidget::itemSelectionChanged, this, &BranchTreeWidget::onSelectionChanged);
   connect(this, &BranchTreeWidget::itemDoubleClicked, this, &BranchTreeWidget::checkoutBranch);
}

void BranchTreeWidget::reloadCurrentBranchLink() const
{
   const auto items = findChildItem(mGit->getCurrentBranch());

   if (!items.isEmpty())
   {
      items.at(0)->setData(0, GitQlient::ShaRole, mGit->getLastCommit().output.toString().trimmed());
      items.at(0)->setData(0, GitQlient::IsCurrentBranchRole, true);
   }
}

int BranchTreeWidget::focusOnBranch(const QString &branch, int lastPos)
{
   const auto items = findChildItem(branch);

   if (lastPos + 1 >= items.count())
      return -1;

   if (lastPos != -1)
   {
      auto itemToUnselect = items.at(lastPos);
      itemToUnselect->setSelected(false);
   }

   ++lastPos;

   auto itemToExpand = items.at(lastPos);
   itemToExpand->setExpanded(true);
   setCurrentItem(itemToExpand);
   setCurrentIndex(indexFromItem(itemToExpand));

   while (itemToExpand->parent())
   {
      itemToExpand->setExpanded(true);
      itemToExpand = itemToExpand->parent();
   }

   itemToExpand->setExpanded(true);

   return lastPos;
}

void BranchTreeWidget::showBranchesContextMenu(const QPoint &pos)
{
   if (const auto item = itemAt(pos); item != nullptr)
   {
      auto selectedBranch = item->data(0, FullNameRole).toString();

      if (!selectedBranch.isEmpty())
      {
         auto currentBranch = mGit->getCurrentBranch();

         const auto menu = new BranchContextMenu({ currentBranch, selectedBranch, mLocal, mGit }, this);
         connect(menu, &BranchContextMenu::signalRefreshPRsCache, this, &BranchTreeWidget::signalRefreshPRsCache);
         connect(menu, &BranchContextMenu::signalFetchPerformed, this, &BranchTreeWidget::signalFetchPerformed);
         connect(menu, &BranchContextMenu::signalBranchesUpdated, this, &BranchTreeWidget::signalBranchesUpdated);
         connect(menu, &BranchContextMenu::signalCheckoutBranch, this, [this, item]() { checkoutBranch(item); });
         connect(menu, &BranchContextMenu::signalMergeRequired, this, &BranchTreeWidget::signalMergeRequired);
         connect(menu, &BranchContextMenu::signalPullConflict, this, &BranchTreeWidget::signalPullConflict);

         menu->exec(viewport()->mapToGlobal(pos));
      }
      else if (item->data(0, IsRoot).toBool())
      {
         const auto menu = new QMenu(this);
         const auto addRemote = menu->addAction(tr("Remove remote"));
         connect(addRemote, &QAction::triggered, this, [this, item]() {
            QScopedPointer<GitRemote> git(new GitRemote(mGit));
            if (const auto ret = git->removeRemote(item->text(0)); ret.success)
               emit signalBranchesUpdated();
         });

         menu->exec(viewport()->mapToGlobal(pos));
      }
   }
   else if (!mLocal)
   {
      const auto menu = new QMenu(this);
      const auto addRemote = menu->addAction(tr("Add remote"));
      connect(addRemote, &QAction::triggered, this, [this]() {
         const auto addRemote = new AddRemoteDlg(mGit);
         const auto ret = addRemote->exec();

         if (ret == QDialog::Accepted)
            emit signalBranchesUpdated();
      });

      menu->exec(viewport()->mapToGlobal(pos));
   }
}

void BranchTreeWidget::checkoutBranch(QTreeWidgetItem *item)
{
   if (item)
   {
      auto branchName = item->data(0, FullNameRole).toString();

      if (!branchName.isEmpty())
      {
         const auto oldItem = findChildItem(mGit->getCurrentBranch());
         const auto isLocal = item->data(0, LocalBranchRole).toBool();
         QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
         QScopedPointer<GitBranches> git(new GitBranches(mGit));
         const auto ret
             = isLocal ? git->checkoutLocalBranch(branchName.remove("origin/")) : git->checkoutRemoteBranch(branchName);
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
            {
               if (!oldItem.empty())
                  oldItem.at(0)->setData(0, GitQlient::IsCurrentBranchRole, false);

               emit signalBranchCheckedOut();
            }
         }
         else
         {
            QMessageBox msgBox(QMessageBox::Critical, tr("Error while checking out"),
                               tr("There were problems during the checkout operation. Please, see the detailed "
                                  "description for more information."),
                               QMessageBox::Ok, this);
            msgBox.setDetailedText(output);
            msgBox.setStyleSheet(GitQlientStyles::getStyles());
            msgBox.exec();
         }
      }
   }
}

void BranchTreeWidget::selectCommit(QTreeWidgetItem *item)
{
   if (item && item->data(0, IsLeaf).toBool())
      emit signalSelectCommit(item->data(0, ShaRole).toString());
}

void BranchTreeWidget::onSelectionChanged()
{
   const auto selection = selectedItems();

   if (!selection.isEmpty())
      selectCommit(selection.constFirst());
}

QList<QTreeWidgetItem *> BranchTreeWidget::findChildItem(const QString &text) const
{
   QModelIndexList indexes = model()->match(model()->index(0, 0, QModelIndex()), GitQlient::FullNameRole, text, -1,
                                            Qt::MatchContains | Qt::MatchRecursive);
   QList<QTreeWidgetItem *> items;
   const int indexesSize = indexes.size();
   items.reserve(indexesSize);
   for (int i = 0; i < indexesSize; ++i)
      items.append(static_cast<QTreeWidgetItem *>(indexes.at(i).internalPointer()));
   return items;
}
