#include <BranchesWidget.h>

#include <AddSubtreeDlg.h>
#include <BranchTreeWidget.h>
#include <BranchesViewDelegate.h>
#include <BranchesWidgetMinimal.h>
#include <ClickableFrame.h>
#include <GitBase.h>
#include <GitCache.h>
#include <GitConfig.h>
#include <GitQlientBranchItemRole.h>
#include <GitQlientSettings.h>
#include <GitStashes.h>
#include <GitSubmodules.h>
#include <GitSubtree.h>
#include <GitTags.h>
#include <RefWidget.h>
#include <StashesContextMenu.h>
#include <SubmodulesContextMenu.h>

#include <QApplication>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

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

struct RemoteBranch
{
   QString remote;
   QString branch;
   QString sha;
};

}

BranchesWidget::BranchesWidget(const QSharedPointer<GitCache> &cache, const QSharedPointer<GitBase> &git,
                               QWidget *parent)
   : QFrame(parent)
   , mCache(cache)
   , mGit(git)
   , mGitTags(new GitTags(mGit))
   , mTagsCount(new QLabel("(0)"))
   , mTagsArrow(new QLabel())
   , mTagsTree(new RefTreeWidget())
   , mStashesList(new QListWidget())
   , mStashesCount(new QLabel(tr("(0)")))
   , mStashesArrow(new QLabel())
   , mSubmodulesCount(new QLabel("(0)"))
   , mSubmodulesArrow(new QLabel())
   , mSubmodulesList(new QListWidget())
   , mSubtreeCount(new QLabel("(0)"))
   , mSubtreeArrow(new QLabel())
   , mSubtreeList(new QListWidget())
   , mMinimize(new QPushButton())
   , mMinimal(new BranchesWidgetMinimal(mCache, mGit))
{
   GitQlientSettings settings(mGit->getGitDir());

   connect(mCache.get(), &GitCache::signalCacheUpdated, this, &BranchesWidget::showBranches);
   connect(mCache.get(), &GitCache::signalCacheUpdated, this, &BranchesWidget::processTags);

   setAttribute(Qt::WA_DeleteOnClose);

   /* LOCAL BRANCHES START*/
   mLocalFrame = new RefWidget(tr("Local"), "LocalHeader", mCache, mGit);
   mLocalFrame->setObjectName("sectionFrame");

   /* LOCAL BRANCHES END */

   /* REMOTE BRANCHES START*/
   mRemoteFrame = new RefWidget(tr("Remote"), "RemoteHeader", mCache, mGit);
   mRemoteFrame->setObjectName("sectionFrame");
   /* REMOTE BRANCHES END */

   /* TAGS START */
   if (const auto visible = settings.localValue("TagsHeader", true).toBool(); !visible)
   {
      const auto icon = QIcon(!visible ? QString(":/icons/add") : QString(":/icons/remove"));
      mTagsArrow->setPixmap(icon.pixmap(QSize(15, 15)));
      mTagsTree->setVisible(visible);
   }
   else
      mTagsArrow->setPixmap(QIcon(":/icons/remove").pixmap(QSize(15, 15)));

   const auto tagsHeaderFrame = new ClickableFrame();
   const auto tagsHeaderLayout = new QHBoxLayout(tagsHeaderFrame);
   tagsHeaderLayout->setContentsMargins(10, 0, 0, 0);
   tagsHeaderLayout->setSpacing(10);
   tagsHeaderLayout->addWidget(new QLabel(tr("Tags")));
   tagsHeaderLayout->addWidget(mTagsCount);
   tagsHeaderLayout->addStretch();
   tagsHeaderLayout->addWidget(mTagsArrow);

   mTagsTree->setColumnCount(1);
   mTagsTree->setMouseTracking(true);
   mTagsTree->setItemDelegate(mTagsDelegate = new BranchesViewDelegate(true));
   mTagsTree->setContextMenuPolicy(Qt::CustomContextMenu);

   const auto tagsLayout = new QVBoxLayout();
   tagsLayout->setContentsMargins(QMargins());
   tagsLayout->setSpacing(0);
   tagsLayout->addWidget(tagsHeaderFrame);
   tagsLayout->addSpacing(5);
   tagsLayout->addWidget(mTagsTree);

   const auto tagsFrame = new QFrame();
   tagsFrame->setObjectName("sectionFrame");
   tagsFrame->setLayout(tagsLayout);
   /* TAGS END */

   /* STASHES START */
   if (const auto visible = settings.localValue("StashesHeader", true).toBool(); !visible)
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
   stashHeaderLayout->addWidget(mStashesCount);
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

   /* SUBMODULES START */
   if (const auto visible = settings.localValue("SubmodulesHeader", true).toBool(); !visible)
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
   /* SUBMODULES END */

   /* SUBTREE START */
   if (const auto visible = settings.localValue("SubtreeHeader", true).toBool(); !visible)
   {
      const auto icon = QIcon(!visible ? QString(":/icons/add") : QString(":/icons/remove"));
      mSubtreeArrow->setPixmap(icon.pixmap(QSize(15, 15)));
      mSubtreeList->setVisible(visible);
   }
   else
      mSubtreeArrow->setPixmap(QIcon(":/icons/remove").pixmap(QSize(15, 15)));

   const auto subtreeHeaderFrame = new ClickableFrame();
   const auto subtreeHeaderLayout = new QHBoxLayout(subtreeHeaderFrame);
   subtreeHeaderLayout->setContentsMargins(10, 0, 0, 0);
   subtreeHeaderLayout->setSpacing(10);
   subtreeHeaderLayout->addWidget(new QLabel(tr("Subtrees")));
   subtreeHeaderLayout->addWidget(mSubtreeCount);
   subtreeHeaderLayout->addStretch();
   subtreeHeaderLayout->addWidget(mSubtreeArrow);

   mSubtreeList->setMouseTracking(true);
   mSubtreeList->setContextMenuPolicy(Qt::CustomContextMenu);

   const auto subtreeLayout = new QVBoxLayout();
   subtreeLayout->setContentsMargins(QMargins());
   subtreeLayout->setSpacing(0);
   subtreeLayout->addWidget(subtreeHeaderFrame);
   subtreeLayout->addSpacing(5);
   subtreeLayout->addWidget(mSubtreeList);

   const auto subtreeFrame = new QFrame();
   subtreeFrame->setObjectName("sectionFrame");
   subtreeFrame->setLayout(subtreeLayout);
   /* SUBTREE END */

   const auto separator = new QFrame();
   separator->setObjectName("separator");

   const auto panelsLayout = new QVBoxLayout();
   panelsLayout->setContentsMargins(QMargins());
   panelsLayout->setSpacing(0);
   panelsLayout->setAlignment(Qt::AlignTop);
   panelsLayout->addWidget(mLocalFrame);
   panelsLayout->addWidget(mRemoteFrame);
   panelsLayout->addWidget(tagsFrame);
   panelsLayout->addWidget(stashFrame);
   panelsLayout->addWidget(submoduleFrame);
   panelsLayout->addWidget(subtreeFrame);
   panelsLayout->addWidget(separator);

   mMinimize->setIcon(QIcon(":/icons/ahead"));
   mMinimize->setToolTip(tr("Show minimalist view"));
   mMinimize->setObjectName("BranchesWidgetOptionsButton");
   mMinimize->setShortcut(Qt::CTRL | Qt::Key_B);
   connect(mMinimize, &QPushButton::clicked, this, &BranchesWidget::minimalView);

   const auto panelsFrame = new QFrame();
   panelsFrame->setObjectName("panelsFrame");
   panelsFrame->setLayout(panelsLayout);

   const auto vLayout = new QVBoxLayout();
   vLayout->setContentsMargins(0, 0, 0, 0);
   vLayout->setSpacing(0);
   vLayout->addWidget(mMinimize);
   vLayout->addSpacing(5);
   vLayout->addWidget(panelsFrame);

   mFullBranchFrame = new QFrame();

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

   const auto isMinimalVisible = settings.localValue("MinimalBranchesView", false).toBool();
   mFullBranchFrame->setVisible(!isMinimalVisible);
   mMinimal->setVisible(isMinimalVisible);
   connect(mMinimal, &BranchesWidgetMinimal::showFullBranchesView, this, &BranchesWidget::fullView);
   connect(mMinimal, &BranchesWidgetMinimal::commitSelected, this, &BranchesWidget::signalSelectCommit);
   connect(mMinimal, &BranchesWidgetMinimal::stashSelected, this, &BranchesWidget::onStashSelected);

   /*
   connect(mLocalBranchesTree, &BranchTreeWidget::signalRefreshPRsCache, mCache.get(),
           &GitCache::refreshPRsCache);
*/
   connect(mLocalFrame, &RefWidget::signalSelectCommit, this, &BranchesWidget::signalSelectCommit);
   connect(mLocalFrame, &RefWidget::signalSelectCommit, mRemoteFrame, &RefWidget::clearSelection);
   connect(mLocalFrame, &RefWidget::fullReload, this, &BranchesWidget::fullReload);
   connect(mLocalFrame, &RefWidget::logReload, this, &BranchesWidget::logReload);
   connect(mLocalFrame, &RefWidget::signalMergeRequired, this, &BranchesWidget::signalMergeRequired);
   connect(mLocalFrame, &RefWidget::mergeSqushRequested, this, &BranchesWidget::mergeSqushRequested);
   connect(mLocalFrame, &RefWidget::signalPullConflict, this, &BranchesWidget::signalPullConflict);

   connect(mRemoteFrame, &RefWidget::signalSelectCommit, this, &BranchesWidget::signalSelectCommit);
   connect(mRemoteFrame, &RefWidget::signalSelectCommit, mLocalFrame, &RefWidget::clearSelection);
   connect(mRemoteFrame, &RefWidget::fullReload, this, &BranchesWidget::fullReload);
   connect(mRemoteFrame, &RefWidget::logReload, this, &BranchesWidget::logReload);
   connect(mRemoteFrame, &RefWidget::signalMergeRequired, this, &BranchesWidget::signalMergeRequired);
   connect(mRemoteFrame, &RefWidget::mergeSqushRequested, this, &BranchesWidget::mergeSqushRequested);

   connect(mTagsTree, &QTreeWidget::itemClicked, this, &BranchesWidget::onTagClicked);
   connect(mTagsTree, &QListWidget::customContextMenuRequested, this, &BranchesWidget::showTagsContextMenu);
   connect(mStashesList, &QListWidget::itemClicked, this, &BranchesWidget::onStashClicked);
   connect(mStashesList, &QListWidget::customContextMenuRequested, this, &BranchesWidget::showStashesContextMenu);
   connect(mSubmodulesList, &QListWidget::customContextMenuRequested, this, &BranchesWidget::showSubmodulesContextMenu);
   connect(mSubtreeList, &QListWidget::customContextMenuRequested, this, &BranchesWidget::showSubtreesContextMenu);
   connect(mLocalFrame, &ClickableFrame::clicked, this, &BranchesWidget::panelsVisibilityChanged);
   connect(mRemoteFrame, &ClickableFrame::clicked, this, &BranchesWidget::panelsVisibilityChanged);
   connect(tagsHeaderFrame, &ClickableFrame::clicked, this, &BranchesWidget::onTagsHeaderClicked);
   connect(stashHeaderFrame, &ClickableFrame::clicked, this, &BranchesWidget::onStashesHeaderClicked);
   connect(submoduleHeaderFrame, &ClickableFrame::clicked, this, &BranchesWidget::onSubmodulesHeaderClicked);
   connect(subtreeHeaderFrame, &ClickableFrame::clicked, this, &BranchesWidget::onSubtreesHeaderClicked);

   connect(mGitTags.get(), &GitTags::remoteTagsReceived, mCache.get(), &GitCache::updateTags);
}

BranchesWidget::~BranchesWidget()
{
   delete mTagsDelegate;
}

bool BranchesWidget::isMinimalViewActive() const
{
   GitQlientSettings settings(mGit->getGitDir());
   return settings.localValue("MinimalBranchesView", false).toBool();
}

void BranchesWidget::showBranches()
{
   QLog_Info("UI", QString("Loading branches data"));

   clear();
   mMinimal->clearActions();

   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

   auto branches = mCache->getBranches(References::Type::LocalBranch);
   std::map<QString, QString> branchShaMap;
   std::map<QString, QString> branchFolderShaMap;

   for (const auto &pair : std::as_const(branches))
   {
      for (const auto &branch : pair.second)
      {
         if (branch.contains("/"))
            branchFolderShaMap.insert(std::make_pair(branch, pair.first));
         else if (!branch.isEmpty())
            branchShaMap.insert(std::make_pair(branch, pair.first));
      }
   }

   if (!branches.empty())
   {
      QLog_Info("UI", QString("Fetched {%1} local branches").arg(branches.count()));
      QLog_Info("UI", QString("Processing local branches..."));

      for (const auto &iter : branchFolderShaMap)
      {
         if (!iter.first.contains("HEAD->"))
         {
            processLocalBranch(iter.second, iter.first);
            mMinimal->configureLocalMenu(iter.second, iter.first);
         }
      }

      auto count = 0;

      for (const auto &iter : branchShaMap)
      {
         if (!iter.first.contains("HEAD->"))
         {
            ++count;
            processLocalBranch(iter.second, iter.first);
            mMinimal->configureLocalMenu(iter.second, iter.first);
         }
      }

      mLocalFrame->setCount(QString("(%1)").arg(count));

      QLog_Info("UI", QString("... local branches processed"));
   }

   branches.clear();
   branches.squeeze();
   branchShaMap.clear();
   branchFolderShaMap.clear();
   branches = mCache->getBranches(References::Type::RemoteBranches);

   QVector<RemoteBranch> remoteBranches;

   for (const auto &pair : std::as_const(branches))
   {
      for (const auto &branch : pair.second)
      {
         remoteBranches.append(
             RemoteBranch { branch.mid(0, branch.indexOf("/")), branch.mid(branch.indexOf("/") + 1), pair.first });
      }
   }

   std::sort(remoteBranches.begin(), remoteBranches.end(), [](const RemoteBranch &r1, const RemoteBranch &r2) {
      return r1.remote == r2.remote ? r1.branch < r2.branch : r1.remote < r2.remote;
   });

   for (const auto &branchInfo : std::as_const(remoteBranches))
   {
      const auto fullBranchName = QString("%1/%2").arg(branchInfo.remote, branchInfo.branch);

      if (branchInfo.branch.contains("/"))
         branchFolderShaMap.insert(std::make_pair(fullBranchName, branchInfo.sha));
      else
         branchShaMap.insert(std::make_pair(fullBranchName, branchInfo.sha));
   }

   if (!branches.empty())
   {
      QLog_Info("UI", QString("Fetched {%1} remote branches").arg(branches.count()));
      QLog_Info("UI", QString("Processing remote branches..."));

      for (const auto &iter : branchFolderShaMap)
      {
         if (!iter.first.contains("HEAD->"))
         {
            processRemoteBranch(iter.second, iter.first);
            mMinimal->configureRemoteMenu(iter.second, iter.first);
         }
      }

      auto count = 0;
      for (const auto &iter : branchShaMap)
      {
         if (!iter.first.contains("HEAD->"))
         {
            ++count;
            processRemoteBranch(iter.second, iter.first);
            mMinimal->configureRemoteMenu(iter.second, iter.first);
         }
      }

      mRemoteFrame->setCount(QString("(%1)").arg(count));

      branches.clear();
      branches.squeeze();

      QLog_Info("UI", QString("... remote branches processed"));
   }

   processStashes();
   processSubmodules();
   processSubtrees();

   QApplication::restoreOverrideCursor();

   mLocalFrame->adjustBranchesTree();
}

void BranchesWidget::refreshCurrentBranchLink()
{
   mLocalFrame->reloadCurrentBranchLink();
}

void BranchesWidget::clear()
{
   blockSignals(true);
   mLocalFrame->clear();
   mRemoteFrame->clear();
   blockSignals(false);
}

void BranchesWidget::fullView()
{
   mFullBranchFrame->setVisible(true);
   mMinimal->setVisible(false);

   emit minimalViewStateChanged(false);

   GitQlientSettings settings(mGit->getGitDir());
   settings.setLocalValue("MinimalBranchesView", mMinimal->isVisible());
}

void BranchesWidget::returnToSavedView()
{
   GitQlientSettings settings(mGit->getGitDir());
   const auto savedState = settings.localValue("MinimalBranchesView", false).toBool();

   if (savedState != mMinimal->isVisible())
   {
      mFullBranchFrame->setVisible(!savedState);
      mMinimal->setVisible(savedState);

      emit minimalViewStateChanged(savedState);
   }
}

void BranchesWidget::minimalView()
{
   forceMinimalView();

   GitQlientSettings settings(mGit->getGitDir());
   settings.setLocalValue("MinimalBranchesView", mMinimal->isVisible());
}

void BranchesWidget::forceMinimalView()
{
   mFullBranchFrame->setVisible(false);
   mMinimal->setVisible(true);

   emit minimalViewStateChanged(true);
}

void BranchesWidget::onPanelsVisibilityChaned()
{
   GitQlientSettings settings(mGit->getGitDir());

   mLocalFrame->reloadVisibility();
   mRemoteFrame->reloadVisibility();

   auto visible = settings.localValue("TagsHeader", true).toBool();
   auto icon = QIcon(!visible ? QString(":/icons/add") : QString(":/icons/remove"));
   mTagsArrow->setPixmap(icon.pixmap(QSize(15, 15)));
   mTagsTree->setVisible(visible);

   visible = settings.localValue("StashesHeader", true).toBool();
   icon = QIcon(!visible ? QString(":/icons/add") : QString(":/icons/remove"));
   mStashesArrow->setPixmap(icon.pixmap(QSize(15, 15)));
   mStashesList->setVisible(visible);

   visible = settings.localValue("SubmodulesHeader", true).toBool();
   icon = QIcon(!visible ? QString(":/icons/add") : QString(":/icons/remove"));
   mSubmodulesArrow->setPixmap(icon.pixmap(QSize(15, 15)));
   mSubmodulesList->setVisible(visible);

   visible = settings.localValue("SubtreeHeader", true).toBool();
   icon = QIcon(!visible ? QString(":/icons/add") : QString(":/icons/remove"));
   mSubtreeArrow->setPixmap(icon.pixmap(QSize(15, 15)));
   mSubtreeList->setVisible(visible);
}

void BranchesWidget::processLocalBranch(const QString &sha, QString branch)
{
   QLog_Debug("UI", QString("Adding local branch {%1}").arg(branch));

   auto isCurrentBranch = false;

   if (branch == mGit->getCurrentBranch())
      isCurrentBranch = true;

   const auto fullBranchName = branch;

   mLocalFrame->addItems(isCurrentBranch, fullBranchName, sha);

   QLog_Debug("UI", QString("Finish gathering local branch information"));
}

void BranchesWidget::processRemoteBranch(const QString &sha, QString branch)
{
   mRemoteFrame->addItems(false, branch, sha);
}

void BranchesWidget::processTags()
{
   mTagsTree->clear();

   const auto localTags = mCache->getTags(References::Type::LocalTag);
   auto remoteTags = mCache->getTags(References::Type::RemoteTag);

   auto tagsCount = 0;

   for (auto iter = localTags.cbegin(); iter != localTags.cend(); ++iter)
   {
      QTreeWidgetItem *parent = nullptr;
      auto fullTagName = iter.key();
      auto folders = fullTagName.split("/");
      auto tagName = folders.takeLast();

      for (const auto &folder : std::as_const(folders))
      {
         QTreeWidgetItem *child = nullptr;

         if (parent)
            child = getChild(parent, folder);
         else
         {
            for (auto i = 0; i < mTagsTree->topLevelItemCount(); ++i)
            {
               if (mTagsTree->topLevelItem(i)->text(0) == folder)
                  child = mTagsTree->topLevelItem(i);
            }
         }

         if (!child)
         {
            const auto item = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem();
            item->setText(0, folder);

            if (!parent)
               mTagsTree->addTopLevelItem(item);

            parent = item;
         }
         else
            parent = child;
      }

      const auto item = new QTreeWidgetItem(parent);

      if (!remoteTags.contains(fullTagName))
      {
         tagName += " (local)";
         item->setData(0, LocalBranchRole, false);
      }
      else
      {
         item->setData(0, LocalBranchRole, true);
         remoteTags.remove(fullTagName);
      }

      QLog_Trace("UI", QString("Adding tag {%1}").arg(tagName));

      item->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicator);
      item->setText(0, tagName);
      item->setData(0, GitQlient::FullNameRole, fullTagName);
      item->setData(0, GitQlient::ShaRole, iter.value());
      item->setData(0, Qt::ToolTipRole, fullTagName);
      item->setData(0, GitQlient::IsLeaf, true);

      mTagsTree->addTopLevelItem(item);

      ++tagsCount;
   }

   for (auto iter = remoteTags.cbegin(); iter != remoteTags.cend(); ++iter)
   {
      QTreeWidgetItem *parent = nullptr;
      auto fullTagName = iter.key();
      auto folders = fullTagName.split("/");
      auto tagName = folders.takeLast();

      for (const auto &folder : std::as_const(folders))
      {
         QTreeWidgetItem *child = nullptr;

         if (parent)
            child = getChild(parent, folder);
         else
         {
            for (auto i = 0; i < mTagsTree->topLevelItemCount(); ++i)
            {
               if (mTagsTree->topLevelItem(i)->text(0) == folder)
                  child = mTagsTree->topLevelItem(i);
            }
         }

         if (!child)
         {
            const auto item = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem();
            item->setText(0, folder);

            if (!parent)
               mTagsTree->addTopLevelItem(item);

            parent = item;
         }
         else
            parent = child;
      }

      QLog_Trace("UI", QString("Adding tag {%1}").arg(tagName));

      const auto item = new QTreeWidgetItem(parent);
      item->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicator);
      item->setText(0, tagName);
      item->setData(0, GitQlient::FullNameRole, fullTagName);
      item->setData(0, GitQlient::ShaRole, iter.value());
      item->setData(0, Qt::ToolTipRole, fullTagName);
      item->setData(0, GitQlient::IsLeaf, true);

      mTagsTree->addTopLevelItem(item);

      ++tagsCount;
   }

   mTagsCount->setText(QString("(%1)").arg(tagsCount));
   mTagsTree->update();
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

void BranchesWidget::processSubtrees()
{
   mSubtreeList->clear();

   QScopedPointer<GitSubtree> git(new GitSubtree(mGit));

   const auto ret = git->list();

   if (ret.success)
   {
      const auto rawData = ret.output;
      const auto commits = rawData.split("\n\n");
      auto count = 0;

      for (auto &subtreeRawData : commits)
      {
         if (!subtreeRawData.isEmpty())
         {
            QString name;
            QString sha;
            auto fields = subtreeRawData.split("\n");

            for (auto &field : fields)
            {
               if (field.contains("git-subtree-dir:"))
                  name = field.remove("git-subtree-dir:").trimmed();
               else if (field.contains("git-subtree-split"))
                  sha = field.remove("git-subtree-split:").trimmed();
            }

            mSubtreeList->addItem(name);
            ++count;
         }
      }

      mSubtreeCount->setText('(' + QString::number(count) + ')');
   }
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
   const auto item = mTagsTree->itemAt(p);

   if (!item)
      return;

   const auto tagName = item->data(0, GitQlient::FullNameRole).toString();

   if (!tagName.isEmpty())
   {
      const auto isRemote = item->data(0, LocalBranchRole).toBool();
      const auto menu = new QMenu(this);
      const auto removeTagAction = menu->addAction(tr("Remove tag"));
      connect(removeTagAction, &QAction::triggered, this, [this, tagName, isRemote]() {
         QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
         QScopedPointer<GitTags> git(new GitTags(mGit));
         const auto ret = git->removeTag(tagName, isRemote);
         QApplication::restoreOverrideCursor();

         if (ret.success)
            mGitTags->getRemoteTags();
      });

      const auto pushTagAction = menu->addAction(tr("Push tag"));
      pushTagAction->setEnabled(!isRemote);
      connect(pushTagAction, &QAction::triggered, this, [this, tagName]() {
         QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
         QScopedPointer<GitTags> git(new GitTags(mGit));
         const auto ret = git->pushTag(tagName);
         QApplication::restoreOverrideCursor();

         if (ret.success)
            mGitTags->getRemoteTags();
      });

      menu->exec(mTagsTree->viewport()->mapToGlobal(p));
   }
}

void BranchesWidget::showStashesContextMenu(const QPoint &p)
{
   QLog_Info("UI", QString("Requesting context menu for stashes"));

   const auto index = mStashesList->indexAt(p);

   if (index.isValid())
   {
      const auto menu = new StashesContextMenu(mGit, index.data(Qt::UserRole).toString(), this);
      connect(menu, &StashesContextMenu::signalUpdateView, this, &BranchesWidget::fullReload);
      connect(menu, &StashesContextMenu::signalContentRemoved, this, &BranchesWidget::fullReload);
      menu->exec(mStashesList->viewport()->mapToGlobal(p));
   }
}

void BranchesWidget::showSubmodulesContextMenu(const QPoint &p)
{
   QLog_Info("UI", QString("Requesting context menu for submodules"));

   const auto menu = new SubmodulesContextMenu(mGit, mSubmodulesList->indexAt(p), mSubmodulesList->count(), this);
   connect(menu, &SubmodulesContextMenu::openSubmodule, this, &BranchesWidget::signalOpenSubmodule);
   connect(menu, &SubmodulesContextMenu::infoUpdated, this, &BranchesWidget::fullReload);

   menu->exec(mSubmodulesList->viewport()->mapToGlobal(p));
}

void BranchesWidget::showSubtreesContextMenu(const QPoint &p)
{
   QLog_Info("UI", QString("Requesting context menu for subtrees"));

   QModelIndex index = mSubtreeList->indexAt(p);

   const auto menu = new QMenu(this);

   if (index.isValid())
   {
      connect(menu->addAction(tr("Pull")), &QAction::triggered, this, [this, index]() {
         QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

         const auto prefix = index.data().toString();
         const auto subtreeData = getSubtreeData(prefix);

         QScopedPointer<GitSubtree> git(new GitSubtree(mGit));
         const auto ret = git->pull(subtreeData.first, subtreeData.second, prefix);
         QApplication::restoreOverrideCursor();

         if (ret.success)
            emit fullReload();
         else
            QMessageBox::warning(this, tr("Error when pulling"), ret.output);
      });
      /*
      connect(menu->addAction(tr("Merge")), &QAction::triggered, this, [this, index]() {
         QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

         const auto subtreeData = getSubtreeData(index.data().toString());

         QScopedPointer<GitSubtree> git(new GitSubtree(mGit));
         const auto ret = git->pull(subtreeData.first, subtreeData.second);
         QApplication::restoreOverrideCursor();

         if (ret.success)
            emit signalBranchesUpdated();
      });
*/
      connect(menu->addAction(tr("Push")), &QAction::triggered, this, [this, index]() {
         QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

         const auto prefix = index.data().toString();
         const auto subtreeData = getSubtreeData(prefix);

         QScopedPointer<GitSubtree> git(new GitSubtree(mGit));
         const auto ret = git->push(subtreeData.first, subtreeData.second, prefix);
         QApplication::restoreOverrideCursor();

         if (ret.success)
            emit fullReload();
         else
            QMessageBox::warning(this, tr("Error when pushing"), ret.output);
      });

      const auto addSubtree = menu->addAction(tr("Configure"));
      connect(addSubtree, &QAction::triggered, this, [this, index]() {
         const auto prefix = index.data().toString();
         const auto subtreeData = getSubtreeData(prefix);
         AddSubtreeDlg addDlg(prefix, subtreeData.first, subtreeData.second, mGit);
         const auto ret = addDlg.exec();
         if (ret == QDialog::Accepted)
            emit fullReload();
      });
      // menu->addAction(tr("Split"));
   }
   else
   {
      const auto addSubtree = menu->addAction(tr("Add subtree"));
      connect(addSubtree, &QAction::triggered, this, [this]() {
         AddSubtreeDlg addDlg(mGit);
         const auto ret = addDlg.exec();
         if (ret == QDialog::Accepted)
            emit fullReload();
      });
   }

   menu->exec(mSubtreeList->viewport()->mapToGlobal(p));
}

void BranchesWidget::onTagsHeaderClicked()
{
   const auto tagsAreVisible = mTagsTree->isVisible();
   const auto icon = QIcon(tagsAreVisible ? QString(":/icons/add") : QString(":/icons/remove"));
   mTagsArrow->setPixmap(icon.pixmap(QSize(15, 15)));
   mTagsTree->setVisible(!tagsAreVisible);

   GitQlientSettings settings(mGit->getGitDir());
   settings.setLocalValue("TagsHeader", !tagsAreVisible);

   emit panelsVisibilityChanged();
}

void BranchesWidget::onStashesHeaderClicked()
{
   const auto stashesAreVisible = mStashesList->isVisible();
   const auto icon = QIcon(stashesAreVisible ? QString(":/icons/add") : QString(":/icons/remove"));
   mStashesArrow->setPixmap(icon.pixmap(QSize(15, 15)));
   mStashesList->setVisible(!stashesAreVisible);

   GitQlientSettings settings(mGit->getGitDir());
   settings.setLocalValue("StashesHeader", !stashesAreVisible);

   emit panelsVisibilityChanged();
}

void BranchesWidget::onSubmodulesHeaderClicked()
{
   const auto submodulesAreVisible = mSubmodulesList->isVisible();
   const auto icon = QIcon(submodulesAreVisible ? QString(":/icons/add") : QString(":/icons/remove"));
   mSubmodulesArrow->setPixmap(icon.pixmap(QSize(15, 15)));
   mSubmodulesList->setVisible(!submodulesAreVisible);

   GitQlientSettings settings(mGit->getGitDir());
   settings.setLocalValue("SubmodulesHeader", !submodulesAreVisible);

   emit panelsVisibilityChanged();
}

void BranchesWidget::onSubtreesHeaderClicked()
{
   const auto subtreesAreVisible = mSubtreeList->isVisible();
   const auto icon = QIcon(subtreesAreVisible ? QString(":/icons/add") : QString(":/icons/remove"));
   mSubtreeArrow->setPixmap(icon.pixmap(QSize(15, 15)));
   mSubtreeList->setVisible(!subtreesAreVisible);

   GitQlientSettings settings(mGit->getGitDir());
   settings.setLocalValue("SubtreeHeader", !subtreesAreVisible);

   emit panelsVisibilityChanged();
}

void BranchesWidget::onTagClicked(QTreeWidgetItem *item)
{
   if (item && item->data(0, IsLeaf).toBool())
      emit signalSelectCommit(item->data(0, ShaRole).toString());
}

void BranchesWidget::onStashClicked(QListWidgetItem *item)
{
   onStashSelected(item->data(Qt::UserRole).toString());
}

void BranchesWidget::onStashSelected(const QString &stashId)
{
   QScopedPointer<GitTags> git(new GitTags(mGit));
   const auto sha = git->getTagCommit(stashId).output;

   emit signalSelectCommit(sha);
}

QPair<QString, QString> BranchesWidget::getSubtreeData(const QString &prefix)
{
   GitQlientSettings settings(mGit->getGitDir());
   bool end = false;
   QString url;
   QString ref;

   for (auto i = 0; !end; ++i)
   {
      const auto repo = settings.localValue(QString("Subtrees/%1.prefix").arg(i), "");

      if (repo.toString() == prefix)
      {
         auto tmpUrl = settings.localValue(QString("Subtrees/%1.url").arg(i)).toString();
         auto tmpRef = settings.localValue(QString("Subtrees/%1.ref").arg(i)).toString();

         if (tmpUrl.isEmpty() || tmpRef.isEmpty())
         {
            const auto resp
                = QMessageBox::question(this, tr("Subtree configuration not found!"),
                                        tr("The subtree configuration was not found. It could be that it was created "
                                           "outside GitQlient.<br>To operate with this subtree, it needs to be "
                                           "configured.<br><br><b>Do you want to configure it now?<b>"));

            if (resp == QMessageBox::Yes)
            {
               AddSubtreeDlg stDlg(prefix, mGit, this);
               const auto ret = stDlg.exec();

               if (ret == QDialog::Accepted)
               {
                  tmpUrl = settings.localValue(QString("Subtrees/%1.url").arg(i)).toString();
                  tmpRef = settings.localValue(QString("Subtrees/%1.ref").arg(i)).toString();

                  if (tmpUrl.isEmpty() || tmpRef.isEmpty())
                     QMessageBox::critical(this, tr("Unexpected error!"),
                                           tr("An unidentified error happened while using subtrees. Please contact the "
                                              "creator of GitQlient for support."));
                  else
                  {
                     url = tmpUrl;
                     ref = tmpRef;
                  }
               }
            }

            end = true;
         }
         else
         {
            url = tmpUrl;
            ref = tmpRef;
            end = true;
         }
      }
      else if (repo.toString().isEmpty())
      {
         const auto resp
             = QMessageBox::question(this, tr("Subtree configuration not found!"),
                                     tr("The subtree configuration was not found. It could be that it was created "
                                        "outside GitQlient.<br>To operate with this subtree, it needs to be "
                                        "configured.<br><br><b>Do you want to configure it now?<b>"));

         if (resp == QMessageBox::Yes)
         {
            AddSubtreeDlg stDlg(prefix, mGit, this);
            const auto ret = stDlg.exec();

            if (ret == QDialog::Accepted)
            {
               const auto tmpUrl = settings.localValue(QString("Subtrees/%1.url").arg(i)).toString();
               const auto tmpRef = settings.localValue(QString("Subtrees/%1.ref").arg(i)).toString();

               if (tmpUrl.isEmpty() || tmpRef.isEmpty())
                  QMessageBox::critical(this, tr("Unexpected error!"),
                                        tr("An unidentified error happened while using subtrees. Please contact the "
                                           "creator of GitQlient for support."));
               else
               {
                  url = tmpUrl;
                  ref = tmpRef;
               }
            }
         }

         end = true;
      }
   }

   return qMakePair(url, ref);
}
