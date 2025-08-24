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
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QRegularExpression>

using namespace GitQlient;

BranchTreeWidget::BranchTreeWidget(QWidget *parent)
   : QTreeWidget(parent)
{
   setHeaderHidden(true);

   setRootIsDecorated(true);
   setIndentation(20);
   setAnimated(true);

   installEventFilter(this);
   setMouseTracking(true);
   setContextMenuPolicy(Qt::CustomContextMenu);

   connect(this, &BranchTreeWidget::customContextMenuRequested,
           this, &BranchTreeWidget::showBranchesContextMenu);
   connect(this, &BranchTreeWidget::itemClicked,
           this, &BranchTreeWidget::selectCommit);
   connect(this, &BranchTreeWidget::itemSelectionChanged,
           this, &BranchTreeWidget::onSelectionChanged);
   connect(this, &BranchTreeWidget::itemDoubleClicked,
           this, &BranchTreeWidget::checkoutBranch);

   const auto delAction = new QAction(this);
   delAction->setShortcut(Qt::Key_Delete);
   connect(delAction, &QAction::triggered, this, &BranchTreeWidget::onDeleteBranch);
   addAction(delAction);
}

BranchTreeWidget::BranchTreeWidget(const QSharedPointer<GitCache> &cache,
                                   const QSharedPointer<GitBase> &git,
                                   QWidget *parent)
   : BranchTreeWidget(parent)
{
   init(cache, git);
}

void BranchTreeWidget::init(const QSharedPointer<GitCache> &cache,
                            const QSharedPointer<GitBase> &git)
{
   mCache = cache;
   mGit = git;
}

void BranchTreeWidget::reloadCurrentBranchLink()
{
   if (!mGit)
      return;

   const auto items = findChildItem(mGit->getCurrentBranch());

   if (!items.isEmpty())
   {
      items.at(0)->setData(0, GitQlient::ShaRole, mGit->getLastCommit().output.trimmed());
      items.at(0)->setData(0, GitQlient::IsCurrentBranchRole, true);
   }
}

void BranchTreeWidget::clearSelection()
{
   QTreeWidget::clearSelection();
}

int BranchTreeWidget::focusOnBranch(const QString &itemText, int startSearchPos)
{
   const auto items = findChildItem(itemText);

   if (items.isEmpty())
      return -1;

   if (startSearchPos >= 0 && startSearchPos < items.count())
   {
      setCurrentItem(items.at(startSearchPos));
      return startSearchPos;
   }
   else if (!items.isEmpty())
   {
      setCurrentItem(items.first());
      return 0;
   }

   return -1;
}

QVector<QTreeWidgetItem *> BranchTreeWidget::findChildItem(const QString &text) const
{
   QVector<QTreeWidgetItem *> items;

   for (int i = 0; i < topLevelItemCount(); ++i)
   {
      auto item = topLevelItem(i);
      if (item->text(0) == text || item->data(0, FullNameRole).toString() == text)
      {
         items.append(item);
      }

      QList<QTreeWidgetItem *> stack;
      stack.append(item);

      while (!stack.isEmpty())
      {
         auto current = stack.takeFirst();
         for (int j = 0; j < current->childCount(); ++j)
         {
            auto child = current->child(j);
            if (child->text(0) == text || child->data(0, FullNameRole).toString() == text)
            {
               items.append(child);
            }
            if (child->childCount() > 0)
            {
               stack.append(child);
            }
         }
      }
   }

   return items;
}

bool BranchTreeWidget::eventFilter(QObject *, QEvent *event)
{
   if (hasFocus() && event->type() == QEvent::KeyRelease)
   {
      if (auto keyEvent = dynamic_cast<QKeyEvent *>(event))
      {
         if (keyEvent->key() == Qt::Key_Delete)
         {
            onDeleteBranch();
            return true;
         }
      }
   }
   return false;
}

void BranchTreeWidget::showBranchesContextMenu(const QPoint &pos)
{
   if (!mCache || !mGit)
      return;

   if (const auto item = itemAt(pos); item != nullptr)
   {
      auto selectedBranch = item->data(0, FullNameRole).toString();

      if (!selectedBranch.isEmpty())
      {
         auto currentBranch = mGit->getCurrentBranch();

         const auto menu = new BranchContextMenu({ currentBranch, selectedBranch, mIsLocal, mCache, mGit }, this);
         connect(menu, &BranchContextMenu::signalRefreshPRsCache,
                 this, &BranchTreeWidget::signalRefreshPRsCache);
         connect(menu, &BranchContextMenu::logReload,
                 this, &BranchTreeWidget::logReload);
         connect(menu, &BranchContextMenu::fullReload,
                 this, &BranchTreeWidget::fullReload);
         connect(menu, &BranchContextMenu::signalCheckoutBranch,
                 this, [this, item]() { checkoutBranch(item); });
         connect(menu, &BranchContextMenu::signalMergeRequired,
                 this, &BranchTreeWidget::signalMergeRequired);
         connect(menu, &BranchContextMenu::mergeSqushRequested,
                 this, &BranchTreeWidget::mergeSqushRequested);
         connect(menu, &BranchContextMenu::signalPullConflict,
                 this, &BranchTreeWidget::signalPullConflict);

         menu->exec(viewport()->mapToGlobal(pos));
      }
      else if (item->data(0, IsRoot).toBool())
      {
         QScopedPointer<GitRemote> git(new GitRemote(mGit));
         if (const auto ret = git->getRemotes(); ret.success)
         {
            const auto remotes = ret.output.split("\n", Qt::SkipEmptyParts);

            if (remotes.count() > 1)
            {
               const auto menu = new QMenu(this);
               const auto removeRemote = menu->addAction(tr("Remove remote"));
               connect(removeRemote, &QAction::triggered, this, [this, item]() {
                  QScopedPointer<GitRemote> git(new GitRemote(mGit));
                  if (const auto ret = git->removeRemote(item->text(0)); ret.success)
                  {
                     mCache->deleteReference(item->data(0, ShaRole).toString(),
                                             References::Type::RemoteBranche,
                                             item->text(0));
                     emit fullReload();
                  }
               });

               menu->exec(viewport()->mapToGlobal(pos));
            }
         }
      }
      else
      {
         GitQlientSettings settings(mGit->getGitDir());
         if (settings.localValue("DeleteRemoteFolder", false).toBool() || mIsLocal)
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
   else if (!mIsLocal)
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
   if (!item || !mGit)
      return;

   auto branchName = item->data(0, FullNameRole).toString();

   if (!branchName.isEmpty())
   {
      const auto isLocal = item->data(0, LocalBranchRole).toBool();
      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
      QScopedPointer<GitBranches> git(new GitBranches(mGit));
      const auto ret = isLocal
          ? git->checkoutLocalBranch(branchName.remove("origin/"))
          : git->checkoutRemoteBranch(branchName);
      QApplication::restoreOverrideCursor();

      const auto output = ret.output;

      if (ret.success)
      {
         static QRegularExpression rx("by \\d+ commits");
         const auto texts = rx.match(output).capturedTexts();
         const auto value = texts.isEmpty() ? QStringList() : texts.constFirst().split(" ");

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
   mFolderToRemove = item;

   const auto menu = new QMenu(this);
   connect(menu->addAction("Delete folder"), &QAction::triggered,
           this, &BranchTreeWidget::deleteFolder);
   menu->exec(viewport()->mapToGlobal(pos));
}

void BranchTreeWidget::discoverBranchesInFolder(QTreeWidgetItem *folder, QStringList &branches)
{
   const auto childrenCount = folder->childCount();

   for (auto i = 0; i < childrenCount; ++i)
   {
      if (const auto fullName = folder->child(i)->data(0, FullNameRole).toString(); !fullName.isEmpty())
         branches.append(fullName);
      else
         discoverBranchesInFolder(folder->child(i), branches);
   }
}

void BranchTreeWidget::deleteFolder()
{
   if (!mFolderToRemove || !mGit || !mCache)
      return;

   QStringList branchesToRemove;
   discoverBranchesInFolder(mFolderToRemove, branchesToRemove);

   auto ret = QMessageBox::warning(
       this, tr("Delete multiple branches!"),
       tr("Are you sure you want to delete the following branches?\n\n%1").arg(branchesToRemove.join("\n")),
       QMessageBox::Ok, QMessageBox::Cancel);

   if (ret == QMessageBox::Ok)
   {
      auto deleted = false;
      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

      for (const auto &branch : std::as_const(branchesToRemove))
      {
         const auto type = mIsLocal ? References::Type::LocalBranch : References::Type::RemoteBranche;
         const auto sha = mCache->getShaOfReference(branch, type);
         QScopedPointer<GitBranches> git(new GitBranches(mGit));
         const auto ret2 = mIsLocal ? git->removeLocalBranch(branch) : git->removeRemoteBranch(branch);

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

   mFolderToRemove = nullptr;
}

void BranchTreeWidget::onDeleteBranch()
{
   if (!mCache || !mGit)
      return;

   const auto items = selectedItems();
   if (items.isEmpty())
      return;

   const auto item = items.constFirst();

   if (item->data(0, IsRoot).toBool())
   {
      auto ret = QMessageBox::warning(this, tr("Delete branch!"),
                                      tr("Are you sure you want to delete the remote?"),
                                      QMessageBox::Ok, QMessageBox::Cancel);

      if (ret == QMessageBox::Ok)
      {
         QScopedPointer<GitRemote> git(new GitRemote(mGit));
         if (const auto ret = git->removeRemote(item->text(0)); ret.success)
         {
            mCache->deleteReference(item->data(0, ShaRole).toString(),
                                    References::Type::RemoteBranche,
                                    item->text(0));
            emit fullReload();
         }
      }
   }
   else if (auto selectedBranch = item->data(0, FullNameRole).toString(); !selectedBranch.isEmpty())
   {
      if (!mIsLocal && selectedBranch == "master")
         QMessageBox::critical(this, tr("Delete master?!"),
                               tr("You are not allowed to delete remote master."),
                               QMessageBox::Ok);
      else
      {
         auto ret = QMessageBox::warning(this, tr("Delete branch!"),
                                         tr("Are you sure you want to delete the branch?"),
                                         QMessageBox::Ok, QMessageBox::Cancel);

         if (ret == QMessageBox::Ok)
         {
            const auto type = mIsLocal ? References::Type::LocalBranch : References::Type::RemoteBranche;
            const auto sha = mCache->getShaOfReference(selectedBranch, type);
            QScopedPointer<GitBranches> git(new GitBranches(mGit));
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            const auto ret2 = mIsLocal ? git->removeLocalBranch(selectedBranch)
                                       : git->removeRemoteBranch(selectedBranch);
            QApplication::restoreOverrideCursor();

            if (ret2.success)
            {
               mCache->deleteReference(sha, type, selectedBranch);
               emit mCache->signalCacheUpdated();
            }
            else
               QMessageBox::critical(
                   this, tr("Delete a branch failed"),
                   tr("There were some problems while deleting the branch:<br><br> %1").arg(ret2.output));
         }
      }
   }
   else
   {
      GitQlientSettings settings(mGit->getGitDir());
      if (settings.localValue("DeleteRemoteFolder", false).toBool() || mIsLocal)
         deleteFolder();
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
