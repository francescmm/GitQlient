#include <RefTreeWidget.h>

#include <BranchTreeWidget.h>
#include <BranchesViewDelegate.h>
#include <ClickableFrame.h>
#include <GitBase.h>
#include <GitCache.h>
#include <GitQlientBranchItemRole.h>
#include <GitQlientSettings.h>

#include <QHeaderView>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

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

RefTreeWidget::RefTreeWidget(const QString &title,
                             const QString &settingsKey,
                             RefType type,
                             const QSharedPointer<GitCache> &cache,
                             const QSharedPointer<GitBase> &git,
                             QWidget *parent)
   : QWidget(parent)
   , mRefType(type)
   , mSettingsKey(settingsKey)
   , mSettings(std::make_unique<GitQlientSettings>(git->getGitDir()))
{
   mFrame = new ClickableFrame(title);
   mFrame->setExpandable(true);
   mFrame->setObjectName("RefWidget");

   mTreeWidget = new BranchTreeWidget(cache, git);
   mTreeWidget->setLocalRepo(type == LocalBranches);
   mTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
   mTreeWidget->setItemDelegate(mDelegate = new BranchesViewDelegate(type == Tags));

   mFrame->setContentWidget(mTreeWidget);

   const bool isExpanded = mSettings->localValue(settingsKey, true).toBool();
   mFrame->setExpanded(isExpanded);

   auto layout = new QVBoxLayout(this);
   layout->setContentsMargins(0, 0, 0, 0);
   layout->addWidget(mFrame);

   connect(mFrame, &ClickableFrame::expanded, this,
           [this](bool expanded) { saveExpansionState(expanded); });

   connect(mTreeWidget, &BranchTreeWidget::signalSelectCommit,
           this, &RefTreeWidget::signalSelectCommit);
   connect(mTreeWidget, &BranchTreeWidget::fullReload,
           this, &RefTreeWidget::fullReload);
   connect(mTreeWidget, &BranchTreeWidget::logReload,
           this, &RefTreeWidget::logReload);
   connect(mTreeWidget, &BranchTreeWidget::signalMergeRequired,
           this, &RefTreeWidget::signalMergeRequired);
   connect(mTreeWidget, &BranchTreeWidget::mergeSqushRequested,
           this, &RefTreeWidget::mergeSqushRequested);
   connect(mTreeWidget, &BranchTreeWidget::signalPullConflict,
           this, &RefTreeWidget::signalPullConflict);

   connect(this, &RefTreeWidget::clearSelection,
           mTreeWidget, &BranchTreeWidget::clearSelection);
}

RefTreeWidget::~RefTreeWidget()
{
   delete mDelegate;
}

void RefTreeWidget::setCount(int count)
{
   mFrame->setCount(count);
}

void RefTreeWidget::reloadCurrentBranchLink()
{
   mTreeWidget->reloadCurrentBranchLink();
}

void RefTreeWidget::clear()
{
   mTreeWidget->clear();
   mFrame->setCount(0);
}

void RefTreeWidget::reloadVisibility()
{
   const bool isExpanded = mSettings->localValue(mSettingsKey, true).toBool();
   mFrame->setExpanded(isExpanded);
}

void RefTreeWidget::saveExpansionState(bool expanded)
{
   mSettings->setLocalValue(mSettingsKey, expanded);
}

void RefTreeWidget::adjustBranchesTree()
{
   mTreeWidget->resizeColumnToContents(0);
}

void RefTreeWidget::addItems(bool isCurrentBranch, const QString &fullBranchName, const QString &sha)
{
   if (mRefType == Tags)
   {
      auto item = new QTreeWidgetItem();
      item->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicator);
      item->setText(0, fullBranchName);
      item->setData(0, GitQlient::FullNameRole, fullBranchName);
      item->setData(0, GitQlient::ShaRole, sha);
      item->setData(0, Qt::ToolTipRole, fullBranchName);
      item->setData(0, GitQlient::IsLeaf, true);
      mTreeWidget->addTopLevelItem(item);
      return;
   }

   QVector<QTreeWidgetItem *> parents;
   QTreeWidgetItem *parent = nullptr;

   auto folders = fullBranchName.split("/");
   auto branch = folders.takeLast();

   for (const auto &folder : std::as_const(folders))
   {
      QTreeWidgetItem *child = nullptr;

      if (parent)
      {
         child = getChild(parent, folder);
         if (child)
            parents.append(child);
      }
      else
      {
         for (auto i = 0; i < mTreeWidget->topLevelItemCount(); ++i)
         {
            if (mTreeWidget->topLevelItem(i)->text(0) == folder)
            {
               child = mTreeWidget->topLevelItem(i);
               parents.append(child);
               break;
            }
         }
      }

      if (!child)
      {
         const auto item = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem();
         item->setText(0, folder);
         item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);

         if (!parent)
            mTreeWidget->addTopLevelItem(item);

         parent = item;
         parents.append(parent);
      }
      else
      {
         parent = child;
      }
   }

   auto item = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem();
   item->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicator);
   item->setText(0, branch);

   item->setData(0, GitQlient::IsCurrentBranchRole, isCurrentBranch);
   item->setData(0, GitQlient::FullNameRole, fullBranchName);
   item->setData(0, GitQlient::LocalBranchRole, mRefType == LocalBranches);
   item->setData(0, GitQlient::ShaRole, sha);
   item->setData(0, Qt::ToolTipRole, fullBranchName);
   item->setData(0, GitQlient::IsLeaf, true);
   item->setData(0, Qt::UserRole, isCurrentBranch);

   if (isCurrentBranch)
   {
      item->setSelected(true);

      for (const auto parent : parents)
      {
         mTreeWidget->setCurrentItem(item);
         mTreeWidget->expandItem(parent);
         const auto indexToScroll = mTreeWidget->currentIndex();
         mTreeWidget->scrollTo(indexToScroll);
      }
   }

   if (!parent)
      mTreeWidget->addTopLevelItem(item);

   parents.clear();
   parents.squeeze();
}
