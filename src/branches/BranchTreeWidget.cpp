#include "BranchTreeWidget.h"

#include <AddRemoteDlg.h>
#include <BranchContextMenu.h>
#include <GitBase.h>
#include <GitBranches.h>
#include <GitCache.h>
#include <GitQlientBranchItemRole.h>
#include <GitQlientSettings.h>
#include <GitQlientStyles.h>
#include <GitRemote.h>
#include <PullDlg.h>

#include <QApplication>
#include <QMessageBox>

using namespace GitQlient;

BranchTreeWidget::BranchTreeWidget(const QSharedPointer<GitCache> &cache, const QSharedPointer<GitBase> &git,
                                   QWidget *parent)
   : RefTreeWidget(parent)
   , mCache(cache)
   , mGit(git)
{

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
      items.at(0)->setData(0, GitQlient::ShaRole, mGit->getLastCommit().output.trimmed());
      items.at(0)->setData(0, GitQlient::IsCurrentBranchRole, true);
   }
}

void BranchTreeWidget::showBranchesContextMenu(const QPoint &pos)
{
   if (const auto item = itemAt(pos); item != nullptr)
   {
      auto selectedBranch = item->data(0, FullNameRole).toString();

      if (!selectedBranch.isEmpty())
      {
         auto currentBranch = mGit->getCurrentBranch();

         const auto menu = new BranchContextMenu({ currentBranch, selectedBranch, mLocal, mCache, mGit }, this);
         connect(menu, &BranchContextMenu::signalRefreshPRsCache, this, &BranchTreeWidget::signalRefreshPRsCache);
         connect(menu, &BranchContextMenu::logReload, this, &BranchTreeWidget::logReload);
         connect(menu, &BranchContextMenu::fullReload, this, &BranchTreeWidget::fullReload);
         connect(menu, &BranchContextMenu::signalCheckoutBranch, this, [this, item]() { checkoutBranch(item); });
         connect(menu, &BranchContextMenu::signalMergeRequired, this, &BranchTreeWidget::signalMergeRequired);
         connect(menu, &BranchContextMenu::mergeSqushRequested, this, &BranchTreeWidget::mergeSqushRequested);
         connect(menu, &BranchContextMenu::signalPullConflict, this, &BranchTreeWidget::signalPullConflict);

         menu->exec(viewport()->mapToGlobal(pos));
      }
      else if (item->data(0, IsRoot).toBool())
      {
         const auto menu = new QMenu(this);
         const auto removeRemote = menu->addAction(tr("Remove remote"));
         connect(removeRemote, &QAction::triggered, this, [this, item]() {
            QScopedPointer<GitRemote> git(new GitRemote(mGit));
            if (const auto ret = git->removeRemote(item->text(0)); ret.success)
            {
               mCache->deleteReference(item->data(0, ShaRole).toString(), References::Type::RemoteBranches,
                                       item->text(0));
               emit logReload();
            }
         });

         menu->exec(viewport()->mapToGlobal(pos));
      }
      else
      {
         GitQlientSettings settings(mGit->getGitDir());
         if (settings.localValue("DeleteRemoteFolder", false).toBool() || mLocal)
            showDeleteFolderMenu(item, pos);
         else
         {
            QMessageBox::warning(this, tr("Delete branch!"),
                                 tr("Deleting multiple remote branches at the same time is disabled in the "
                                    "configuration of GitQlient.\n\n"
                                    "To enable, go to the Configuration panel, Repository tab."),
                                 QMessageBox::Ok);
         }
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
            emit fullReload();
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
         const auto isLocal = item->data(0, LocalBranchRole).toBool();
         QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
         QScopedPointer<GitBranches> git(new GitBranches(mGit));
         const auto ret
             = isLocal ? git->checkoutLocalBranch(branchName.remove("origin/")) : git->checkoutRemoteBranch(branchName);
         QApplication::restoreOverrideCursor();

         const auto output = ret.output;

         if (ret.success)
         {
            QRegExp rx("by \\d+ commits");
            rx.indexIn(output);
            auto value = rx.capturedTexts().constFirst().split(" ");
            auto uiUpdateRequested = false;

            if (value.count() == 3 && output.contains("your branch is behind", Qt::CaseInsensitive))
            {
               PullDlg pull(mGit, output.split('\n').first());
               connect(&pull, &PullDlg::signalRepositoryUpdated, this, &BranchTreeWidget::fullReload);
               connect(&pull, &PullDlg::signalPullConflict, this, &BranchTreeWidget::signalPullConflict);

               if (pull.exec() == QDialog::Accepted)
                  uiUpdateRequested = true;
            }

            if (!uiUpdateRequested)
            {
               if (auto oldItem = findChildItem(mGit->getCurrentBranch()); !oldItem.empty())
               {
                  oldItem.at(0)->setData(0, GitQlient::IsCurrentBranchRole, false);
                  oldItem.clear();
                  oldItem.squeeze();
               }
            }

            emit fullReload();
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

void BranchTreeWidget::showDeleteFolderMenu(QTreeWidgetItem *item, const QPoint &pos)
{
   const auto childrenCount = item->childCount();
   QStringList branchesToRemove;

   for (auto i = 0; i < childrenCount; ++i)
      branchesToRemove.append(item->child(i)->data(0, FullNameRole).toString());

   const auto menu = new QMenu(this);
   connect(menu->addAction("Delete folder"), &QAction::triggered, this, [this, branchesToRemove]() {
      auto ret = QMessageBox::warning(
          this, tr("Delete branch!"),
          tr("Are you sure you want to delete the following branches?\n\n%1").arg(branchesToRemove.join("\n")),
          QMessageBox::Ok, QMessageBox::Cancel);

      if (ret == QMessageBox::Ok)
      {
         auto deleted = false;
         QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

         for (const auto &branch : branchesToRemove)
         {
            const auto type = mLocal ? References::Type::LocalBranch : References::Type::RemoteBranches;
            const auto sha = mCache->getShaOfReference(branch, type);
            QScopedPointer<GitBranches> git(new GitBranches(mGit));
            const auto ret2 = mLocal ? git->removeLocalBranch(branch) : git->removeRemoteBranch(branch);

            if (ret2.success)
            {
               mCache->deleteReference(sha, type, branch);
               deleted = true;
            }
            else
               QMessageBox::critical(
                   this, tr("Delete a branch failed"),
                   tr("There were some problems while deleting the branch:<br><br> %1").arg(ret2.output));
         }

         QApplication::restoreOverrideCursor();

         if (deleted)
         {
            emit mCache->signalCacheUpdated();
            emit logReload();
         }
      }
   });

   menu->exec(viewport()->mapToGlobal(pos));
}
