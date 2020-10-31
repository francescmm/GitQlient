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
#include <SubmodulesContextMenu.h>
#include <GitCache.h>
#include <GitQlientBranchItemRole.h>
#include <GitQlientSettings.h>
#include <BranchesWidgetMinimal.h>

#include <QApplication>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QListWidget>
#include <QLabel>
#include <QMenu>
#include <QHeaderView>
#include <QPushButton>
#include <QToolButton>

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
   , mGitTags(new GitTags(mGit))
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
   connect(mGitTags.data(), &GitTags::remoteTagsReceived, mCache.data(), &GitCache::updateTags);
   connect(mCache.get(), &GitCache::signalCacheUpdated, this, &BranchesWidget::processTags);

   setAttribute(Qt::WA_DeleteOnClose);

   mLocalBranchesTree->setLocalRepo(true);
   mLocalBranchesTree->setMouseTracking(true);
   mLocalBranchesTree->setItemDelegate(new BranchesViewDelegate());
   mLocalBranchesTree->setColumnCount(2);
   mLocalBranchesTree->setObjectName("LocalBranches");

   const auto localHeader = mLocalBranchesTree->headerItem();
   localHeader->setText(0, tr("Local"));
   localHeader->setText(1, tr("To origin"));

   mRemoteBranchesTree->setColumnCount(1);
   mRemoteBranchesTree->setMouseTracking(true);
   mRemoteBranchesTree->setItemDelegate(new BranchesViewDelegate());

   const auto remoteHeader = mRemoteBranchesTree->headerItem();
   remoteHeader->setText(0, tr("Remote"));

   /* TAGS */
   GitQlientSettings settings;
   if (const auto visible = settings.localValue(mGit->getGitQlientSettingsDir(), "TagsHeader", true).toBool(); !visible)
   {
      const auto icon = QIcon(!visible ? QString(":/icons/add") : QString(":/icons/remove"));
      mTagArrow->setPixmap(icon.pixmap(QSize(15, 15)));
      mTagsList->setVisible(visible);
   }
   else
      mTagArrow->setPixmap(QIcon(":/icons/remove").pixmap(QSize(15, 15)));

   const auto tagsHeaderFrame = new ClickableFrame();
   const auto tagsHeaderLayout = new QHBoxLayout(tagsHeaderFrame);
   tagsHeaderLayout->setContentsMargins(10, 0, 0, 0);
   tagsHeaderLayout->setSpacing(10);
   tagsHeaderLayout->addWidget(new QLabel(tr("Tags")));
   tagsHeaderLayout->addWidget(mTagsCount);
   tagsHeaderLayout->addStretch();
   tagsHeaderLayout->addWidget(mTagArrow);

   const auto tagLayout = new QVBoxLayout();
   tagLayout->setContentsMargins(QMargins());
   tagLayout->setSpacing(0);
   tagLayout->addWidget(tagsHeaderFrame);
   tagLayout->addSpacing(5);
   tagLayout->addWidget(mTagsList);

   const auto tagsFrame = new QFrame();
   tagsFrame->setObjectName("sectionFrame");
   tagsFrame->setLayout(tagLayout);

   mTagsList->setMouseTracking(true);
   mTagsList->setContextMenuPolicy(Qt::CustomContextMenu);

   /* TAGS END */

   /* STASHES */
   if (const auto visible = settings.localValue(mGit->getGitQlientSettingsDir(), "StashesHeader", true).toBool();
       !visible)
   {
      const auto icon = QIcon(!visible ? QString(":/icons/add") : QString(":/icons/remove"));
      mStashesArrow->setPixmap(icon.pixmap(QSize(15, 15)));
      mStashesList->setVisible(visible);
   }
   else
      mStashesArrow->setPixmap(QIcon(":/icons/remove").pixmap(QSize(15, 15)));

   const auto stashHeaderFrame = new ClickableFrame();
   const auto stashHeaderLayout = new QHBoxLayout(stashHeaderFrame);
   stashHeaderLayout->setContentsMargins(10, 0, 0, 0);
   stashHeaderLayout->setSpacing(10);
   stashHeaderLayout->addWidget(new QLabel(tr("Stashes")));
   stashHeaderLayout->addWidget(mStashesCount = new QLabel(tr("(0)")));
   stashHeaderLayout->addStretch();
   stashHeaderLayout->addWidget(mStashesArrow);

   mStashesList->setMouseTracking(true);
   mStashesList->setContextMenuPolicy(Qt::CustomContextMenu);

   const auto stashLayout = new QVBoxLayout();
   stashLayout->setContentsMargins(QMargins());
   stashLayout->setSpacing(0);
   stashLayout->addWidget(stashHeaderFrame);
   stashLayout->addSpacing(5);
   stashLayout->addWidget(mStashesList);

   const auto stashFrame = new QFrame();
   stashFrame->setObjectName("sectionFrame");
   stashFrame->setLayout(stashLayout);

   /* STASHES END */
   if (const auto visible = settings.localValue(mGit->getGitQlientSettingsDir(), "SubmodulesHeader", true).toBool();
       !visible)
   {
      const auto icon = QIcon(!visible ? QString(":/icons/add") : QString(":/icons/remove"));
      mSubmodulesArrow->setPixmap(icon.pixmap(QSize(15, 15)));
      mSubmodulesList->setVisible(visible);
   }
   else
      mSubmodulesArrow->setPixmap(QIcon(":/icons/remove").pixmap(QSize(15, 15)));

   const auto submoduleHeaderFrame = new ClickableFrame();
   const auto submoduleHeaderLayout = new QHBoxLayout(submoduleHeaderFrame);
   submoduleHeaderLayout->setContentsMargins(10, 0, 0, 0);
   submoduleHeaderLayout->setSpacing(10);
   submoduleHeaderLayout->addWidget(new QLabel(tr("Submodules")));
   submoduleHeaderLayout->addWidget(mSubmodulesCount);
   submoduleHeaderLayout->addStretch();
   submoduleHeaderLayout->addWidget(mSubmodulesArrow);

   mSubmodulesList->setMouseTracking(true);
   mSubmodulesList->setContextMenuPolicy(Qt::CustomContextMenu);
   connect(mSubmodulesList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
      emit signalOpenSubmodule(mGit->getWorkingDir().append("/").append(item->text()));
   });

   const auto submoduleLayout = new QVBoxLayout();
   submoduleLayout->setContentsMargins(QMargins());
   submoduleLayout->setSpacing(0);
   submoduleLayout->addWidget(submoduleHeaderFrame);
   submoduleLayout->addSpacing(5);
   submoduleLayout->addWidget(mSubmodulesList);

   const auto submoduleFrame = new QFrame();
   submoduleFrame->setObjectName("sectionFrame");
   submoduleFrame->setLayout(submoduleLayout);

   /* SUBMODULES START */

   /* SUBMODULES END */

   const auto searchBranch = new QLineEdit();
   searchBranch->setPlaceholderText(tr("Prese ENTER to search a branch..."));
   searchBranch->setObjectName("SearchInput");
   connect(searchBranch, &QLineEdit::returnPressed, this, &BranchesWidget::onSearchBranch);

   mMinimize->setIcon(QIcon(":/icons/ahead"));
   mMinimize->setToolTip(tr("Show minimalist view"));
   mMinimize->setObjectName("BranchesWidgetOptionsButton");
   connect(mMinimize, &QPushButton::clicked, this, &BranchesWidget::minimalView);

   const auto mainControlsLayout = new QHBoxLayout();
   mainControlsLayout->setContentsMargins(QMargins());
   mainControlsLayout->setSpacing(5);
   mainControlsLayout->addWidget(mMinimize);
   mainControlsLayout->addWidget(searchBranch);

   const auto panelsLayout = new QVBoxLayout();
   panelsLayout->setContentsMargins(QMargins());
   panelsLayout->setSpacing(0);
   panelsLayout->addWidget(mLocalBranchesTree);
   panelsLayout->addWidget(mRemoteBranchesTree);
   panelsLayout->addWidget(tagsFrame);
   panelsLayout->addWidget(stashFrame);
   panelsLayout->addWidget(submoduleFrame);

   const auto panelsFrame = new QFrame();
   panelsFrame->setObjectName("panelsFrame");
   panelsFrame->setLayout(panelsLayout);

   const auto vLayout = new QVBoxLayout();
   vLayout->setContentsMargins(0, 0, 10, 0);
   vLayout->setSpacing(0);
   vLayout->addLayout(mainControlsLayout);
   vLayout->addSpacing(5);
   vLayout->addWidget(panelsFrame);

   mFullBranchFrame = new QFrame();
   mFullBranchFrame->setObjectName("FullBranchesWidget");
   const auto mainBranchLayout = new QHBoxLayout(mFullBranchFrame);
   mainBranchLayout->setContentsMargins(QMargins());
   mainBranchLayout->setSpacing(0);
   mainBranchLayout->addLayout(vLayout);

   const auto mainLayout = new QGridLayout(this);
   mainLayout->setContentsMargins(QMargins());
   mainLayout->setSpacing(0);
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
   connect(mLocalBranchesTree, &BranchTreeWidget::signalFetchPerformed, mGitTags.data(), &GitTags::getRemoteTags);
   connect(mLocalBranchesTree, &BranchTreeWidget::signalBranchesUpdated, this, &BranchesWidget::signalBranchesUpdated);
   connect(mLocalBranchesTree, &BranchTreeWidget::signalBranchCheckedOut, this,
           &BranchesWidget::signalBranchCheckedOut);
   connect(mLocalBranchesTree, &BranchTreeWidget::signalMergeRequired, this, &BranchesWidget::signalMergeRequired);
   connect(mLocalBranchesTree, &BranchTreeWidget::signalPullConflict, this, &BranchesWidget::signalPullConflict);
   connect(mRemoteBranchesTree, &BranchTreeWidget::signalSelectCommit, this, &BranchesWidget::signalSelectCommit);
   connect(mRemoteBranchesTree, &BranchTreeWidget::signalSelectCommit, mLocalBranchesTree,
           &BranchTreeWidget::clearSelection);
   connect(mRemoteBranchesTree, &BranchTreeWidget::signalFetchPerformed, mGitTags.data(), &GitTags::getRemoteTags);
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
   connect(tagsHeaderFrame, &ClickableFrame::clicked, this, &BranchesWidget::onTagsHeaderClicked);
   connect(stashHeaderFrame, &ClickableFrame::clicked, this, &BranchesWidget::onStashesHeaderClicked);
   connect(submoduleHeaderFrame, &ClickableFrame::clicked, this, &BranchesWidget::onSubmodulesHeaderClicked);
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

      item->setText(1, QString("%1 \u2193 - %2 \u2191").arg(distances.behindOrigin).arg(distances.aheadOrigin));
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
   mTagsList->clear();

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
   mStashesList->clear();

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
   mSubmodulesList->clear();

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

   const auto menu = new SubmodulesContextMenu(mGit, mSubmodulesList->indexAt(p), this);
   connect(menu, &SubmodulesContextMenu::openSubmodule, this, &BranchesWidget::signalOpenSubmodule);
   connect(menu, &SubmodulesContextMenu::infoUpdated, this, &BranchesWidget::signalBranchesUpdated);

   menu->exec(mSubmodulesList->viewport()->mapToGlobal(p));
}

void BranchesWidget::onTagsHeaderClicked()
{
   const auto tagsAreVisible = mTagsList->isVisible();
   const auto icon = QIcon(tagsAreVisible ? QString(":/icons/add") : QString(":/icons/remove"));
   mTagArrow->setPixmap(icon.pixmap(QSize(15, 15)));
   mTagsList->setVisible(!tagsAreVisible);

   GitQlientSettings settings;
   settings.setLocalValue(mGit->getGitQlientSettingsDir(), "TagsHeader", !tagsAreVisible);
}

void BranchesWidget::onStashesHeaderClicked()
{
   const auto stashesAreVisible = mStashesList->isVisible();
   const auto icon = QIcon(stashesAreVisible ? QString(":/icons/add") : QString(":/icons/remove"));
   mStashesArrow->setPixmap(icon.pixmap(QSize(15, 15)));
   mStashesList->setVisible(!stashesAreVisible);

   GitQlientSettings settings;
   settings.setLocalValue(mGit->getGitQlientSettingsDir(), "StashesHeader", !stashesAreVisible);
}

void BranchesWidget::onSubmodulesHeaderClicked()
{
   const auto submodulesAreVisible = mSubmodulesList->isVisible();
   const auto icon = QIcon(submodulesAreVisible ? QString(":/icons/add") : QString(":/icons/remove"));
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

void BranchesWidget::onStashSelected(const QString &stashId)
{
   QScopedPointer<GitTags> git(new GitTags(mGit));
   const auto sha = git->getTagCommit(stashId).output.toString();

   emit signalSelectCommit(sha);
}

void BranchesWidget::onSearchBranch()
{
   const auto lineEdit = qobject_cast<QLineEdit *>(sender());

   const auto text = lineEdit->text();

   if (mLastSearch != text)
   {
      mLastSearch = text;
      mLastIndex = mLocalBranchesTree->focusOnBranch(text);
      mLastTreeSearched = mLocalBranchesTree;

      if (mLastIndex == -1)
      {
         mLastIndex = mRemoteBranchesTree->focusOnBranch(mLastSearch);
         mLastTreeSearched = mRemoteBranchesTree;

         if (mLastIndex == -1)
            mLastTreeSearched = mLocalBranchesTree;
      }
   }
   else
   {
      if (mLastTreeSearched == mLocalBranchesTree)
      {
         if (mLastIndex != -1)
         {
            mLastIndex = mLocalBranchesTree->focusOnBranch(mLastSearch, mLastIndex);
            mLastTreeSearched = mLocalBranchesTree;
         }

         if (mLastIndex == -1)
         {
            mLastIndex = mRemoteBranchesTree->focusOnBranch(mLastSearch);
            mLastTreeSearched = mRemoteBranchesTree;
         }
      }
      else if (mLastIndex != -1)
      {
         mLastIndex = mRemoteBranchesTree->focusOnBranch(mLastSearch, mLastIndex);
         mLastTreeSearched = mRemoteBranchesTree;

         if (mLastIndex == -1)
            mLastTreeSearched = mLocalBranchesTree;
      }
   }
}
