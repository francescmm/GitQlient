#include "BranchesWidget.h"

#include <BranchTreeWidget.h>
#include <GitBase.h>
#include <GitSubmodules.h>
#include <GitStashes.h>
#include <GitTags.h>
#include <GitConfig.h>
#include <BranchesViewDelegate.h>
#include <ClickableFrame.h>
#include <AddSubmoduleDlg.h>
#include <StashesContextMenu.h>
#include <GitCache.h>
#include <GitQlientBranchItemRole.h>
#include <GitQlientSettings.h>
#include <BranchesWidgetMinimal.h>

#include <QApplication>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QListWidget>
#include <QLabel>
#include <QMenu>
#include <QHeaderView>
#include <QPushButton>

#include <QLogger.h>

using namespace QLogger;
using namespace GitQlient;

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

BranchesWidget::BranchesWidget(const QSharedPointer<GitCache> &cache, const QSharedPointer<GitBase> &git,
                               QWidget *parent)
   : QFrame(parent)
   , mCache(cache)
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
   , mMinimize(new QPushButton())
   , mMinimal(new BranchesWidgetMinimal(mCache, mGit))
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

   const auto tagLayout = new QVBoxLayout();
   tagLayout->setContentsMargins(QMargins());
   tagLayout->setSpacing(0);
   tagLayout->addWidget(tagsFrame);
   tagLayout->addWidget(mTagsList);

   GitQlientSettings settings;
   if (const auto visible = settings.localValue(mGit->getGitQlientSettingsDir(), "TagsHeader", true).toBool(); !visible)
   {
      mTagsList->setVisible(!visible);
      onTagsHeaderClicked();
   }

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

   /* TAGS END */

   /* STASHES */

   const auto stashFrame = new ClickableFrame();
   stashFrame->setObjectName("tagsFrame");

   const auto stashLayout = new QVBoxLayout();
   stashLayout->setContentsMargins(QMargins());
   stashLayout->setSpacing(0);
   stashLayout->addWidget(stashFrame);
   stashLayout->addWidget(mStashesList);

   if (const auto visible = settings.localValue(mGit->getGitQlientSettingsDir(), "StashesHeader", true).toBool();
       !visible)
   {
      mStashesList->setVisible(!visible);
      onStashesHeaderClicked();
   }

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

   /* STASHES END */

   const auto submoduleFrame = new ClickableFrame();
   submoduleFrame->setObjectName("tagsFrame");

   const auto submoduleLayout = new QVBoxLayout();
   submoduleLayout->setContentsMargins(QMargins());
   submoduleLayout->setSpacing(0);
   submoduleLayout->addWidget(submoduleFrame);
   submoduleLayout->addWidget(mSubmodulesList);

   if (const auto visible = settings.localValue(mGit->getGitQlientSettingsDir(), "SubmodulesHeader", true).toBool();
       !visible)
   {
      mSubmodulesList->setVisible(!visible);
      onSubmodulesHeaderClicked();
   }

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
   connect(mSubmodulesList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
      emit signalOpenSubmodule(mGit->getWorkingDir().append("/").append(item->text()));
   });

   /* SUBMODULES START */

   /* SUBMODULES END */

   const auto vLayout = new QVBoxLayout();
   vLayout->setContentsMargins(0, 0, 10, 0);
   vLayout->setSpacing(0);
   vLayout->addWidget(mLocalBranchesTree);
   vLayout->addWidget(mRemoteBranchesTree);
   vLayout->addLayout(tagLayout);
   vLayout->addLayout(stashLayout);
   vLayout->addLayout(submoduleLayout);

   mMinimize->setObjectName("MinimizeBtn");
   mMinimize->setIcon(QIcon(":/icons/arrow_right"));
   mMinimize->setToolTip(tr("Show minimalist view"));
   connect(mMinimize, &QPushButton::clicked, this, &BranchesWidget::minimalView);

   mFullBranchFrame = new QFrame();
   mFullBranchFrame->setObjectName("FullBranchesWidget");
   const auto mainBranchLayout = new QHBoxLayout(mFullBranchFrame);
   mainBranchLayout->setContentsMargins(QMargins());
   mainBranchLayout->setSpacing(0);
   mainBranchLayout->addWidget(mMinimize);
   mainBranchLayout->addLayout(vLayout);

   const auto mainLayout = new QGridLayout(this);
   mainLayout->setContentsMargins(QMargins());
   mainLayout->setSpacing(10);
   mainLayout->addWidget(mFullBranchFrame, 0, 0, 3, 1);
   mainLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Fixed, QSizePolicy::Expanding), 0, 1);
   mainLayout->addWidget(mMinimal, 1, 1);
   mainLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Fixed, QSizePolicy::Expanding), 2, 1);

   const auto isMinimalVisible
       = settings.localValue(mGit->getGitQlientSettingsDir(), "MinimalBranchesView", false).toBool();
   mFullBranchFrame->setVisible(!isMinimalVisible);
   mMinimal->setVisible(isMinimalVisible);
   connect(mMinimal, &BranchesWidgetMinimal::showFullBranchesView, this, &BranchesWidget::fullView);
   connect(mMinimal, &BranchesWidgetMinimal::commitSelected, this, &BranchesWidget::signalSelectCommit);
   connect(mMinimal, &BranchesWidgetMinimal::stashSelected, this, &BranchesWidget::onStashSelected);

   /*
   connect(mLocalBranchesTree, &BranchTreeWidget::signalRefreshPRsCache, mCache.get(),
           &GitCache::refreshPRsCache);
*/
   connect(mLocalBranchesTree, &BranchTreeWidget::signalSelectCommit, this, &BranchesWidget::signalSelectCommit);
   connect(mLocalBranchesTree, &BranchTreeWidget::signalSelectCommit, mRemoteBranchesTree,
           &BranchTreeWidget::clearSelection);
   connect(mLocalBranchesTree, &BranchTreeWidget::signalFetchPerformed, this, &BranchesWidget::onFetchPerformed);
   connect(mLocalBranchesTree, &BranchTreeWidget::signalBranchesUpdated, this, &BranchesWidget::signalBranchesUpdated);
   connect(mLocalBranchesTree, &BranchTreeWidget::signalBranchCheckedOut, this,
           &BranchesWidget::signalBranchCheckedOut);
   connect(mLocalBranchesTree, &BranchTreeWidget::signalMergeRequired, this, &BranchesWidget::signalMergeRequired);
   connect(mLocalBranchesTree, &BranchTreeWidget::signalPullConflict, this, &BranchesWidget::signalPullConflict);
   connect(mRemoteBranchesTree, &BranchTreeWidget::signalSelectCommit, this, &BranchesWidget::signalSelectCommit);
   connect(mRemoteBranchesTree, &BranchTreeWidget::signalSelectCommit, mLocalBranchesTree,
           &BranchTreeWidget::clearSelection);
   connect(mRemoteBranchesTree, &BranchTreeWidget::signalFetchPerformed, this, &BranchesWidget::onFetchPerformed);
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
   mMinimal->clearActions();

   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

   auto branches = mCache->getBranches(References::Type::LocalBranch);

   if (!branches.empty())
   {
      QLog_Info("UI", QString("Fetched {%1} local branches").arg(branches.count()));
      QLog_Info("UI", QString("Processing local branches..."));

      for (const auto &pair : branches)
      {
         for (const auto &branch : pair.second)
         {
            if (!branch.contains("HEAD->"))
            {
               processLocalBranch(pair.first, branch);
               mMinimal->configureLocalMenu(pair.first, branch);
            }
         }
      }

      QLog_Info("UI", QString("... local branches processed"));
   }

   branches = mCache->getBranches(References::Type::RemoteBranches);

   if (!branches.empty())
   {
      QLog_Info("UI", QString("Fetched {%1} remote branches").arg(branches.count()));
      QLog_Info("UI", QString("Processing remote branches..."));

      for (const auto &pair : qAsConst(branches))
      {
         for (const auto &branch : pair.second)
         {
            if (!branch.contains("HEAD->"))
            {
               processRemoteBranch(pair.first, branch);
               mMinimal->configureRemoteMenu(pair.first, branch);
            }
         }
      }

      QLog_Info("UI", QString("... rmote branches processed"));
   }

   processTags();
   processStashes();
   processSubmodules();

   QApplication::restoreOverrideCursor();

   adjustBranchesTree(mLocalBranchesTree);
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

void BranchesWidget::fullView()
{
   mFullBranchFrame->setVisible(true);
   mMinimal->setVisible(false);

   GitQlientSettings settings;
   settings.setLocalValue(mGit->getGitQlientSettingsDir(), "MinimalBranchesView", mMinimal->isVisible());
}

void BranchesWidget::returnToSavedView()
{
   GitQlientSettings settings;
   const auto savedState = settings.localValue(mGit->getGitQlientSettingsDir(), "MinimalBranchesView", false).toBool();

   if (savedState != mMinimal->isVisible())
   {
      mFullBranchFrame->setVisible(!savedState);
      mMinimal->setVisible(savedState);
   }
}

void BranchesWidget::minimalView()
{
   forceMinimalView();

   GitQlientSettings settings;
   settings.setLocalValue(mGit->getGitQlientSettingsDir(), "MinimalBranchesView", mMinimal->isVisible());
}

void BranchesWidget::forceMinimalView()
{
   mFullBranchFrame->setVisible(false);
   mMinimal->setVisible(true);
}

void BranchesWidget::processLocalBranch(const QString &sha, QString branch)
{
   QLog_Debug("UI", QString("Adding local branch {%1}").arg(branch));

   auto isCurrentBranch = false;

   if (branch == mGit->getCurrentBranch())
      isCurrentBranch = true;

   const auto fullBranchName = branch;

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
   item->setData(0, GitQlient::IsCurrentBranchRole, isCurrentBranch);
   item->setData(0, GitQlient::FullNameRole, fullBranchName);
   item->setData(0, GitQlient::LocalBranchRole, true);
   item->setData(0, GitQlient::ShaRole, sha);
   item->setData(0, Qt::ToolTipRole, fullBranchName);
   item->setData(0, GitQlient::IsLeaf, true);

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

   if (fullBranchName != "detached")
   {
      const auto distances = mCache->getLocalBranchDistances(fullBranchName);

      item->setText(1, QString("%1 \u2193 - %2 \u2191").arg(distances.behindMaster).arg(distances.aheadMaster));
      item->setText(2, QString("%1 \u2193 - %2 \u2191").arg(distances.behindOrigin).arg(distances.aheadOrigin));
   }

   mLocalBranchesTree->addTopLevelItem(item);

   QLog_Debug("UI", QString("Finish gathering local branch information"));
}

void BranchesWidget::processRemoteBranch(const QString &sha, QString branch)
{
   const auto fullBranchName = branch;
   auto folders = branch.split("/");
   branch = folders.takeLast();

   QTreeWidgetItem *parent = nullptr;

   for (const auto &folder : qAsConst(folders))
   {
      QTreeWidgetItem *child = nullptr;

      if (parent)
         child = getChild(parent, folder);
      else
      {
         for (auto i = 0; i < mRemoteBranchesTree->topLevelItemCount(); ++i)
         {
            if (mRemoteBranchesTree->topLevelItem(i)->text(0) == folder)
               child = mRemoteBranchesTree->topLevelItem(i);
         }
      }

      if (!child)
      {
         const auto item = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem();
         item->setText(0, folder);

         if (!parent)
            mRemoteBranchesTree->addTopLevelItem(item);

         parent = item;
      }
      else
         parent = child;
   }

   QLog_Debug("UI", QString("Adding remote branch {%1}").arg(branch));

   const auto item = new QTreeWidgetItem(parent);
   item->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicator);
   item->setText(0, branch);
   item->setData(0, GitQlient::FullNameRole, fullBranchName);
   item->setData(0, GitQlient::LocalBranchRole, false);
   item->setData(0, GitQlient::ShaRole, sha);
   item->setData(0, Qt::ToolTipRole, fullBranchName);
   item->setData(0, GitQlient::IsLeaf, true);
}

void BranchesWidget::processTags()
{
   const auto localTags = mCache->getTags(References::Type::LocalTag);
   const auto remoteTags = mCache->getTags(References::Type::RemoteTag);

   QLog_Info("UI", QString("Fetching {%1} tags").arg(localTags.count()));

   for (const auto &tag : localTags.toStdMap())
   {
      auto tagName = tag.first;
      const auto item = new QListWidgetItem();
      item->setData(Qt::UserRole, tagName);
      item->setData(Qt::UserRole + 1, true);
      item->setData(Qt::UserRole + 2, tag.second);

      if (!remoteTags.contains(tagName))
      {
         tagName += " (local)";
         item->setData(Qt::UserRole + 1, false);
      }

      item->setText(tagName);
      mTagsList->addItem(item);

      mMinimal->configureTagsMenu(tag.second, tagName);
   }

   mTagsCount->setText(QString("(%1)").arg(localTags.count()));
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
      mMinimal->configureStashesMenu(stashId, stashDesc);
   }

   mStashesCount->setText(QString("(%1)").arg(stashes.count()));
}

void BranchesWidget::processSubmodules()
{
   QScopedPointer<GitSubmodules> git(new GitSubmodules(mGit));
   const auto submodules = git->getSubmodules();

   QLog_Info("UI", QString("Fetching {%1} submodules").arg(submodules.count()));

   for (const auto &submodule : submodules)
   {
      mSubmodulesList->addItem(submodule);
      mMinimal->configureSubmodulesMenu(submodule);
   }

   mSubmodulesCount->setText('(' + QString::number(submodules.count()) + ')');
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
      connect(openSubmoduleAction, &QAction::triggered, this, [this, submoduleName]() {
         emit signalOpenSubmodule(mGit->getWorkingDir().append("/").append(submoduleName));
      });

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

   GitQlientSettings settings;
   settings.setLocalValue(mGit->getGitQlientSettingsDir(), "TagsHeader", !tagsAreVisible);
}

void BranchesWidget::onStashesHeaderClicked()
{
   const auto stashesAreVisible = mStashesList->isVisible();
   const auto icon = QIcon(stashesAreVisible ? QString(":/icons/arrow_up") : QString(":/icons/arrow_down"));
   mStashesArrow->setPixmap(icon.pixmap(QSize(15, 15)));
   mStashesList->setVisible(!stashesAreVisible);

   GitQlientSettings settings;
   settings.setLocalValue(mGit->getGitQlientSettingsDir(), "StashesHeader", !stashesAreVisible);
}

void BranchesWidget::onSubmodulesHeaderClicked()
{
   const auto submodulesAreVisible = mSubmodulesList->isVisible();
   const auto icon = QIcon(submodulesAreVisible ? QString(":/icons/arrow_up") : QString(":/icons/arrow_down"));
   mSubmodulesArrow->setPixmap(icon.pixmap(QSize(15, 15)));
   mSubmodulesList->setVisible(!submodulesAreVisible);

   GitQlientSettings settings;
   settings.setLocalValue(mGit->getGitQlientSettingsDir(), "SubmodulesHeader", !submodulesAreVisible);
}

void BranchesWidget::onTagClicked(QListWidgetItem *item)
{
   emit signalSelectCommit(item->data(Qt::UserRole + 2).toString());
}

void BranchesWidget::onStashClicked(QListWidgetItem *item)
{
   onStashSelected(item->data(Qt::UserRole).toString());
}

void BranchesWidget::onFetchPerformed()
{
   QScopedPointer<GitTags> gitTags(new GitTags(mGit));
   const auto remoteTags = gitTags->getRemoteTags();

   mCache->updateTags(remoteTags);
}

void BranchesWidget::onStashSelected(const QString &stashId)
{
   QScopedPointer<GitTags> git(new GitTags(mGit));
   const auto sha = git->getTagCommit(stashId).output.toString();

   emit signalSelectCommit(sha);
}
