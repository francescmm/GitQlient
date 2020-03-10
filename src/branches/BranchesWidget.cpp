#include "BranchesWidget.h"

#include <BranchTreeWidget.h>
#include <GitBranches.h>
#include <GitTags.h>
#include <GitSubmodules.h>
#include <GitStashes.h>
#include <BranchesViewDelegate.h>
#include <ClickableFrame.h>
#include <AddSubmoduleDlg.h>
#include <StashesContextMenu.h>

#include <QApplication>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QListWidget>
#include <QLabel>
#include <QMenu>
#include <QHeaderView>

#include <QLogger.h>

using namespace QLogger;

namespace
{
QTreeWidgetItem *getChild(QTreeWidgetItem *parent, const QString &childName)
{
   QTreeWidgetItem *child = nullptr;

   if (parent)
   {
      const auto childrenCount = parent->childCount();

      for (auto i = 0; i < childrenCount; ++i)
         if (parent->child(i)->text(0) == childName)
            child = parent->child(i);
   }

   return child;
}
}

BranchesWidget::BranchesWidget(const QSharedPointer<GitBase> &git, QWidget *parent)
   : QFrame(parent)
   , mGit(git)
   , mLocalBranchesTree(new BranchTreeWidget(mGit))
   , mRemoteBranchesTree(new BranchTreeWidget(mGit))
   , mTagsList(new QListWidget())
   , mStashesList(new QListWidget())
   , mSubmodulesList(new QListWidget())
   , mTagsCount(new QLabel("(0)"))
   , mTagArrow(new QLabel())
   , mStashesArrow(new QLabel())
   , mSubmodulesCount(new QLabel("(0)"))
   , mSubmodulesArrow(new QLabel())
{
   setAttribute(Qt::WA_DeleteOnClose);

   mLocalBranchesTree->setLocalRepo(true);
   mLocalBranchesTree->setMouseTracking(true);
   mLocalBranchesTree->setItemDelegate(new BranchesViewDelegate());
   mLocalBranchesTree->setColumnCount(3);

   const auto localHeader = mLocalBranchesTree->headerItem();
   localHeader->setText(0, QString("   %1").arg(tr("Local")));
   localHeader->setIcon(0, QIcon(":/icons/local"));
   localHeader->setText(1, tr("Master"));
   localHeader->setText(2, tr("Origin"));

   mRemoteBranchesTree->setColumnCount(1);
   mRemoteBranchesTree->setMouseTracking(true);
   mRemoteBranchesTree->setItemDelegate(new BranchesViewDelegate());

   const auto remoteHeader = mRemoteBranchesTree->headerItem();
   remoteHeader->setText(0, QString("   %1").arg(tr("Remote")));
   remoteHeader->setIcon(0, QIcon(":/icons/server"));

   /* TAGS */

   const auto tagsFrame = new ClickableFrame();
   tagsFrame->setObjectName("tagsFrame");
   const auto tagsHeaderLayout = new QHBoxLayout(tagsFrame);
   tagsHeaderLayout->setContentsMargins(20, 9, 10, 9);
   tagsHeaderLayout->setSpacing(10);

   const auto tagsIcon = new QLabel();
   tagsIcon->setPixmap(QIcon(":/icons/tags").pixmap(QSize(15, 15)));

   tagsHeaderLayout->addWidget(tagsIcon);
   tagsHeaderLayout->addWidget(new QLabel(tr("Tags")));
   tagsHeaderLayout->addWidget(mTagsCount);
   tagsHeaderLayout->addStretch();

   mTagArrow->setPixmap(QIcon(":/icons/arrow_down").pixmap(QSize(15, 15)));
   tagsHeaderLayout->addWidget(mTagArrow);

   mTagsList->setMouseTracking(true);
   mTagsList->setContextMenuPolicy(Qt::CustomContextMenu);

   const auto tagLayout = new QVBoxLayout();
   tagLayout->setContentsMargins(QMargins());
   tagLayout->setSpacing(0);
   tagLayout->addWidget(tagsFrame);
   tagLayout->addWidget(mTagsList);

   /* TAGS END */

   /* STASHES */

   const auto stashFrame = new ClickableFrame();
   stashFrame->setObjectName("tagsFrame");

   const auto stashHeaderLayout = new QHBoxLayout(stashFrame);
   stashHeaderLayout->setContentsMargins(20, 9, 10, 9);
   stashHeaderLayout->setSpacing(10);

   const auto stashIconLabel = new QLabel();
   stashIconLabel->setPixmap(QIcon(":/icons/stashes").pixmap(QSize(15, 15)));

   stashHeaderLayout->addWidget(stashIconLabel);
   stashHeaderLayout->addWidget(new QLabel(tr("Stashes")));
   stashHeaderLayout->addWidget(mStashesCount = new QLabel(tr("(0)")));
   stashHeaderLayout->addStretch();

   mStashesArrow->setPixmap(QIcon(":/icons/arrow_down").pixmap(QSize(15, 15)));
   stashHeaderLayout->addWidget(mStashesArrow);

   mStashesList->setMouseTracking(true);
   mStashesList->setContextMenuPolicy(Qt::CustomContextMenu);

   const auto stashLayout = new QVBoxLayout();
   stashLayout->setContentsMargins(QMargins());
   stashLayout->setSpacing(0);
   stashLayout->addWidget(stashFrame);
   stashLayout->addWidget(mStashesList);

   /* STASHES END */

   const auto submoduleFrame = new ClickableFrame();
   submoduleFrame->setObjectName("tagsFrame");

   const auto submoduleHeaderLayout = new QHBoxLayout(submoduleFrame);
   submoduleHeaderLayout->setContentsMargins(20, 9, 10, 9);
   submoduleHeaderLayout->setSpacing(10);

   const auto submoduleIconLabel = new QLabel();
   submoduleIconLabel->setPixmap(QIcon(":/icons/submodules").pixmap(QSize(15, 15)));

   submoduleHeaderLayout->addWidget(submoduleIconLabel);
   submoduleHeaderLayout->addWidget(new QLabel(tr("Submodules")));
   submoduleHeaderLayout->addWidget(mSubmodulesCount);
   submoduleHeaderLayout->addStretch();

   mSubmodulesArrow->setPixmap(QIcon(":/icons/arrow_down").pixmap(QSize(15, 15)));

   submoduleHeaderLayout->addWidget(mSubmodulesArrow);

   mSubmodulesList->setMouseTracking(true);
   mSubmodulesList->setContextMenuPolicy(Qt::CustomContextMenu);
   connect(mSubmodulesList, &QListWidget::itemDoubleClicked, this,
           [this](QListWidgetItem *item) { emit signalOpenSubmodule(item->text()); });

   const auto submoduleLayout = new QVBoxLayout();
   submoduleLayout->setContentsMargins(QMargins());
   submoduleLayout->setSpacing(0);
   submoduleLayout->addWidget(submoduleFrame);
   submoduleLayout->addWidget(mSubmodulesList);

   /* SUBMODULES START */

   /* SUBMODULES END */

   const auto vLayout = new QVBoxLayout(this);
   vLayout->setContentsMargins(QMargins());
   vLayout->addWidget(mLocalBranchesTree);
   vLayout->addWidget(mRemoteBranchesTree);
   vLayout->addLayout(tagLayout);
   vLayout->addLayout(stashLayout);
   vLayout->addLayout(submoduleLayout);

   setLayout(vLayout);

   connect(mLocalBranchesTree, &BranchTreeWidget::signalSelectCommit, this, &BranchesWidget::signalSelectCommit);
   connect(mLocalBranchesTree, &BranchTreeWidget::signalSelectCommit, mRemoteBranchesTree,
           &BranchTreeWidget::clearSelection);
   connect(mLocalBranchesTree, &BranchTreeWidget::signalBranchesUpdated, this, &BranchesWidget::signalBranchesUpdated);
   connect(mLocalBranchesTree, &BranchTreeWidget::signalBranchCheckedOut, this,
           &BranchesWidget::signalBranchCheckedOut);
   connect(mLocalBranchesTree, &BranchTreeWidget::signalMergeRequired, this, &BranchesWidget::signalMergeRequired);
   connect(mRemoteBranchesTree, &BranchTreeWidget::signalSelectCommit, this, &BranchesWidget::signalSelectCommit);
   connect(mRemoteBranchesTree, &BranchTreeWidget::signalSelectCommit, mLocalBranchesTree,
           &BranchTreeWidget::clearSelection);
   connect(mRemoteBranchesTree, &BranchTreeWidget::signalBranchesUpdated, this, &BranchesWidget::signalBranchesUpdated);
   connect(mRemoteBranchesTree, &BranchTreeWidget::signalBranchCheckedOut, this,
           &BranchesWidget::signalBranchCheckedOut);
   connect(mRemoteBranchesTree, &BranchTreeWidget::signalMergeRequired, this, &BranchesWidget::signalMergeRequired);
   connect(mTagsList, &QListWidget::itemClicked, this, &BranchesWidget::onTagClicked);
   connect(mTagsList, &QListWidget::customContextMenuRequested, this, &BranchesWidget::showTagsContextMenu);
   connect(mTagsList, &QListWidget::customContextMenuRequested, this, &BranchesWidget::showTagsContextMenu);
   connect(mStashesList, &QListWidget::itemClicked, this, &BranchesWidget::onStashClicked);
   connect(mStashesList, &QListWidget::customContextMenuRequested, this, &BranchesWidget::showStashesContextMenu);
   connect(mSubmodulesList, &QListWidget::customContextMenuRequested, this, &BranchesWidget::showSubmodulesContextMenu);
   connect(tagsFrame, &ClickableFrame::clicked, this, &BranchesWidget::onTagsHeaderClicked);
   connect(stashFrame, &ClickableFrame::clicked, this, &BranchesWidget::onStashesHeaderClicked);
   connect(submoduleFrame, &ClickableFrame::clicked, this, &BranchesWidget::onSubmodulesHeaderClicked);
}

void BranchesWidget::showBranches()
{
   QLog_Info("UI", QString("Loading branches data"));

   clear();

   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   QScopedPointer<GitBranches> git(new GitBranches(mGit));
   auto ret = git->getBranches();

   if (ret.success)
   {
      auto output = ret.output.toString();
      if (!output.startsWith("fatal"))
      {
         output.replace(' ', "");
         const auto branches = output.split('\n');

         QLog_Info("UI", QString("Fetched {%1} branches").arg(branches.count()));
         QLog_Info("UI", QString("Processing branches..."));

         mRemoteBranchesTree->addTopLevelItem(new QTreeWidgetItem({ "origin" }));

         for (const auto &branch : branches)
         {
            if (!branch.isEmpty())
            {
               if (branch.startsWith("remotes/") && !branch.contains("HEAD->"))
                  processRemoteBranch(branch);
               else if (!branch.contains("HEAD->"))
                  processLocalBranch(branch);
            }
         }

         QLog_Info("UI", QString("... branches processed"));
      }

      processTags();
      processStashes();
      processSubmodules();

      QApplication::restoreOverrideCursor();

      adjustBranchesTree(mLocalBranchesTree);
   }
}

void BranchesWidget::clear()
{
   blockSignals(true);
   mLocalBranchesTree->clear();
   mRemoteBranchesTree->clear();
   mTagsList->clear();
   mStashesList->clear();
   mSubmodulesList->clear();
   blockSignals(false);
}

void BranchesWidget::processLocalBranch(QString branch)
{
   QLog_Debug("UI", QString("Adding local branch {%1}").arg(branch));

   auto fullBranchName = branch;
   auto isCurrentBranch = false;
   QString sha;

   if (branch.startsWith('*'))
   {
      branch.replace('*', "");
      fullBranchName.replace('*', "");
      isCurrentBranch = true;

      if (fullBranchName.startsWith("(HEADdetachedat"))
      {
         auto shortSha = fullBranchName.remove("(HEADdetachedat");
         sha = shortSha.remove(")");

         fullBranchName = "detached";
         branch = "detached";
      }
   }
   else
   {
      QScopedPointer<GitBranches> git(new GitBranches(mGit));
      sha = git->getLastCommitOfBranch(fullBranchName).output.toString();
   }

   QVector<QTreeWidgetItem *> parents;
   QTreeWidgetItem *parent = nullptr;
   auto folders = branch.split("/");
   branch = folders.takeLast();

   for (const auto &folder : qAsConst(folders))
   {
      QTreeWidgetItem *child = nullptr;

      if (parent)
      {
         child = getChild(parent, folder);
         parents.append(child);
      }
      else
      {
         for (auto i = 0; i < mLocalBranchesTree->topLevelItemCount(); ++i)
         {
            if (mLocalBranchesTree->topLevelItem(i)->text(0) == folder)
            {
               child = mLocalBranchesTree->topLevelItem(i);
               parents.append(child);
            }
         }
      }

      if (!child)
      {
         const auto item = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem();
         item->setText(0, folder);

         if (!parent)
            mLocalBranchesTree->addTopLevelItem(item);

         parent = item;
         parents.append(parent);
      }
      else
      {
         parent = child;
         parents.append(child);
      }
   }

   auto item = new QTreeWidgetItem(parent);
   item->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicator);
   item->setText(0, branch);
   item->setData(0, Qt::UserRole, isCurrentBranch);
   item->setData(0, Qt::UserRole + 1, fullBranchName);
   item->setData(0, Qt::UserRole + 2, true);
   item->setData(0, Qt::UserRole + 3, sha);
   item->setData(0, Qt::ToolTipRole, fullBranchName);

   if (isCurrentBranch)
   {
      item->setSelected(true);

      for (const auto parent : parents)
      {
         mLocalBranchesTree->setCurrentItem(item);
         mLocalBranchesTree->expandItem(parent);
         const auto indexToScroll = mLocalBranchesTree->currentIndex();
         mLocalBranchesTree->scrollTo(indexToScroll);
      }
   }

   QLog_Debug("UI", QString("Calculating distances..."));

   QScopedPointer<GitBranches> git(new GitBranches(mGit));

   auto distanceToMaster = QString("Not found");
   auto distanceToOrigin = QString("Local");

   if (fullBranchName != "detached")
   {
      const auto toMaster = git->getDistanceBetweenBranches(true, fullBranchName).output.toString();

      if (!toMaster.contains("fatal"))
      {
         distanceToMaster.replace('\n', "");
         distanceToMaster.replace('\t', "\u2193 - ");
         distanceToMaster.append("\u2191");
      }

      const auto toOrigin = git->getDistanceBetweenBranches(false, fullBranchName).output.toString();

      if (!toOrigin.contains("fatal"))
      {
         distanceToOrigin = toOrigin;
         distanceToOrigin.replace('\n', "");
         distanceToOrigin.append("\u2193");
      }
   }

   item->setText(1, distanceToMaster);
   item->setText(2, distanceToOrigin);

   mLocalBranchesTree->addTopLevelItem(item);

   QLog_Debug("UI", QString("Finish gathering local branch information"));
}

void BranchesWidget::processRemoteBranch(QString branch)
{
   branch.replace("remotes/", "");

   const auto fullBranchName = branch;

   branch.replace("origin/", "");

   auto parent = mRemoteBranchesTree->topLevelItem(0);
   auto folders = branch.split("/");
   branch = folders.takeLast();

   auto found = false;

   for (const auto &folder : folders)
   {
      const auto children = parent->childCount();
      if (children != 0)
      {
         for (auto i = 0; i < children; ++i)
         {
            if (parent->child(i)->data(0, Qt::DisplayRole) == folder)
            {
               parent = parent->child(i);
               found = true;
               break;
            }
         }
      }
      if (!found)
      {
         auto item = new QTreeWidgetItem(parent);
         item->setText(0, folder);
         parent = item;
      }
   }

   QLog_Debug("UI", QString("Adding remote branch {%1}").arg(branch));

   const auto item = new QTreeWidgetItem(parent);
   item->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicator);
   item->setText(0, branch);
   item->setData(0, Qt::UserRole + 1, fullBranchName);
   item->setData(0, Qt::UserRole + 2, true);
   item->setData(0, Qt::ToolTipRole, fullBranchName);
}

void BranchesWidget::processTags()
{
   QScopedPointer<GitTags> git(new GitTags(mGit));
   const auto tags = git->getTags();
   const auto localTags = git->getLocalTags();

   QLog_Info("UI", QString("Fetching {%1} tags").arg(tags.count()));

   for (auto tag : tags)
   {
      const auto item = new QListWidgetItem();
      item->setData(Qt::UserRole, tag);
      item->setData(Qt::UserRole + 1, true);

      if (localTags.contains(tag))
      {
         tag += " (local)";
         item->setData(Qt::UserRole + 1, false);
      }

      item->setText(tag);
      mTagsList->addItem(item);
   }

   mTagsCount->setText(QString("(%1)").arg(tags.count()));
}

void BranchesWidget::processStashes()
{
   QScopedPointer<GitStashes> git(new GitStashes(mGit));
   const auto stashes = git->getStashes();

   QLog_Info("UI", QString("Fetching {%1} stashes").arg(stashes.count()));

   for (const auto &stash : stashes)
   {
      const auto stashId = stash.split(":").first();
      const auto stashDesc = stash.split("}: ").last();
      const auto item = new QListWidgetItem(stashDesc);
      item->setData(Qt::UserRole, stashId);
      mStashesList->addItem(item);
   }

   mStashesCount->setText(QString("(%1)").arg(stashes.count()));
}

void BranchesWidget::processSubmodules()
{
   QScopedPointer<GitSubmodules> git(new GitSubmodules(mGit));
   const auto submodules = git->getSubmodules();

   QLog_Info("UI", QString("Fetching {%1} submodules").arg(submodules.count()));

   for (const auto &submodule : submodules)
      mSubmodulesList->addItem(submodule);
}

void BranchesWidget::adjustBranchesTree(BranchTreeWidget *treeWidget)
{
   for (auto i = 1; i < treeWidget->columnCount(); ++i)
      treeWidget->resizeColumnToContents(i);

   treeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);

   for (auto i = 1; i < treeWidget->columnCount(); ++i)
      treeWidget->header()->setSectionResizeMode(i, QHeaderView::ResizeToContents);

   treeWidget->header()->setStretchLastSection(false);
}

void BranchesWidget::showTagsContextMenu(const QPoint &p)
{
   QModelIndex index = mTagsList->indexAt(p);

   if (!index.isValid())
      return;

   const auto tagName = index.data(Qt::UserRole).toString();
   const auto isRemote = index.data(Qt::UserRole + 1).toBool();
   const auto menu = new QMenu(this);
   const auto removeTagAction = menu->addAction(tr("Remove tag"));
   connect(removeTagAction, &QAction::triggered, this, [this, tagName, isRemote]() {
      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
      QScopedPointer<GitTags> git(new GitTags(mGit));
      const auto ret = git->removeTag(tagName, isRemote);
      QApplication::restoreOverrideCursor();

      if (ret.success)
         emit signalBranchesUpdated();
   });

   const auto pushTagAction = menu->addAction(tr("Push tag"));
   pushTagAction->setEnabled(!isRemote);
   connect(pushTagAction, &QAction::triggered, this, [this, tagName]() {
      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
      QScopedPointer<GitTags> git(new GitTags(mGit));
      const auto ret = git->pushTag(tagName);
      QApplication::restoreOverrideCursor();

      if (ret.success)
         emit signalBranchesUpdated();
   });

   menu->exec(mTagsList->viewport()->mapToGlobal(p));
}

void BranchesWidget::showStashesContextMenu(const QPoint &p)
{
   QLog_Info("UI", QString("Requesting context menu for stashes"));

   const auto index = mStashesList->indexAt(p);

   if (index.isValid())
   {
      const auto menu = new StashesContextMenu(mGit, index.data(Qt::UserRole).toString(), this);
      connect(menu, &StashesContextMenu::signalUpdateView, this, &BranchesWidget::signalBranchesUpdated);
      connect(menu, &StashesContextMenu::signalContentRemoved, this, &BranchesWidget::signalBranchesUpdated);
      menu->exec(mStashesList->viewport()->mapToGlobal(p));
   }
}

void BranchesWidget::showSubmodulesContextMenu(const QPoint &p)
{
   QLog_Info("UI", QString("Requesting context menu for submodules"));

   const auto index = mSubmodulesList->indexAt(p);
   const auto menu = new QMenu(this);

   if (!index.isValid())
   {
      const auto addSubmoduleAction = menu->addAction(tr("Add submodule"));
      connect(addSubmoduleAction, &QAction::triggered, this, [this] {
         const auto git = QSharedPointer<GitSubmodules>::create(mGit);
         AddSubmoduleDlg addDlg(git);
         const auto ret = addDlg.exec();
         if (ret == QDialog::Accepted)
            emit signalBranchesUpdated();
      });
   }
   else
   {
      const auto submoduleName = index.data().toString();
      const auto updateSubmoduleAction = menu->addAction(tr("Update"));
      connect(updateSubmoduleAction, &QAction::triggered, this, [this, submoduleName]() {
         QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
         QScopedPointer<GitSubmodules> git(new GitSubmodules(mGit));
         const auto ret = git->submoduleUpdate(submoduleName);
         QApplication::restoreOverrideCursor();

         if (ret)
            emit signalBranchesUpdated();
      });

      const auto openSubmoduleAction = menu->addAction(tr("Open"));
      connect(openSubmoduleAction, &QAction::triggered, this,
              [this, submoduleName]() { emit signalOpenSubmodule(submoduleName); });

      /*
      const auto deleteSubmoduleAction = menu->addAction(tr("Delete"));
      connect(deleteSubmoduleAction, &QAction::triggered, this, []() {});
      */
   }

   menu->exec(mSubmodulesList->viewport()->mapToGlobal(p));
}

void BranchesWidget::onTagsHeaderClicked()
{
   const auto tagsAreVisible = mTagsList->isVisible();
   const auto icon = QIcon(tagsAreVisible ? QString(":/icons/arrow_up") : QString(":/icons/arrow_down"));
   mTagArrow->setPixmap(icon.pixmap(QSize(15, 15)));
   mTagsList->setVisible(!tagsAreVisible);
}

void BranchesWidget::onStashesHeaderClicked()
{
   const auto stashesAreVisible = mStashesList->isVisible();
   const auto icon = QIcon(stashesAreVisible ? QString(":/icons/arrow_up") : QString(":/icons/arrow_down"));
   mStashesArrow->setPixmap(icon.pixmap(QSize(15, 15)));
   mStashesList->setVisible(!stashesAreVisible);
}

void BranchesWidget::onSubmodulesHeaderClicked()
{
   const auto submodulesAreVisible = mSubmodulesList->isVisible();
   const auto icon = QIcon(submodulesAreVisible ? QString(":/icons/arrow_up") : QString(":/icons/arrow_down"));
   mSubmodulesArrow->setPixmap(icon.pixmap(QSize(15, 15)));
   mSubmodulesList->setVisible(!submodulesAreVisible);
}

void BranchesWidget::onTagClicked(QListWidgetItem *item)
{
   QScopedPointer<GitTags> git(new GitTags(mGit));
   const auto sha = git->getTagCommit(item->text()).output.toString();

   emit signalSelectCommit(sha);
}

void BranchesWidget::onStashClicked(QListWidgetItem *item)
{
   QScopedPointer<GitTags> git(new GitTags(mGit));
   const auto sha = git->getTagCommit(item->data(Qt::UserRole).toString()).output.toString();

   emit signalSelectCommit(sha);
}
