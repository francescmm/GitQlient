#include <RefWidget.h>

#include <BranchesViewDelegate.h>
#include <GitBase.h>
#include <GitQlientBranchItemRole.h>

#include "ui_RefWidget.h"

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

RefWidget::RefWidget(const QString& header, const QString& settingsKey, const QSharedPointer<GitCache> &cache, const QSharedPointer<GitBase> &git, QWidget *parent)
   : ClickableFrame(parent)
   , ui(new Ui::RefWidget)
   , mSettingsKey(settingsKey)
   , mSettings(GitQlientSettings(git->getGitDir()))
{
   ui->setupUi(this);

   ui->treeWidget->init(cache, git);
   ui->lName->setText(header);

   if (const auto visible = mSettings.localValue("LocalHeader", true).toBool(); !visible)
   {
      const auto icon = QIcon(!visible ? QString(":/icons/add") : QString(":/icons/remove"));
      ui->lArrow->setPixmap(icon.pixmap(QSize(15, 15)));
      ui->treeWidget->setVisible(visible);
   }
   else
      ui->lArrow->setPixmap(QIcon(":/icons/remove").pixmap(QSize(15, 15)));

   ui->treeWidget->setLocalRepo(true);
   ui->treeWidget->setMouseTracking(true);
   ui->treeWidget->setItemDelegate(mLocalDelegate = new BranchesViewDelegate());
   ui->treeWidget->setColumnCount(1);
   ui->treeWidget->setObjectName("LocalBranches");

   setObjectName("sectionFrame");

   connect(this, &ClickableFrame::clicked, this, &RefWidget::onHeaderClicked);
   connect(ui->treeWidget, &BranchTreeWidget::signalSelectCommit, this, &RefWidget::signalSelectCommit);
   connect(ui->treeWidget, &BranchTreeWidget::fullReload, this, &RefWidget::fullReload);
   connect(ui->treeWidget, &BranchTreeWidget::logReload, this, &RefWidget::logReload);
   connect(ui->treeWidget, &BranchTreeWidget::signalMergeRequired, this, &RefWidget::signalMergeRequired);
   connect(ui->treeWidget, &BranchTreeWidget::mergeSqushRequested, this, &RefWidget::mergeSqushRequested);
   connect(ui->treeWidget, &BranchTreeWidget::signalPullConflict, this, &RefWidget::signalPullConflict);
   connect(this, &RefWidget::clearSelection, ui->treeWidget, &BranchTreeWidget::clearSelection);
}

RefWidget::~RefWidget()
{
   delete ui;
   delete mLocalDelegate;
}

void RefWidget::setCount(const QString& count)
{
   ui->lCount->setText(count);
}

void RefWidget::adjustBranchesTree()
{
   for (auto i = 1; i < ui->treeWidget->columnCount(); ++i)
      ui->treeWidget->resizeColumnToContents(i);

   ui->treeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);

   for (auto i = 1; i < ui->treeWidget->columnCount(); ++i)
      ui->treeWidget->header()->setSectionResizeMode(i, QHeaderView::ResizeToContents);

   ui->treeWidget->header()->setStretchLastSection(false);
}

void RefWidget::reloadCurrentBranchLink()
{
   ui->treeWidget->reloadCurrentBranchLink();
}

void RefWidget::clear()
{
   ui->treeWidget->clear();
}

void RefWidget::reloadVisibility()
{
   auto visible = mSettings.localValue(mSettingsKey, true).toBool();
   auto icon = QIcon(!visible ? QString(":/icons/add") : QString(":/icons/remove"));
   ui->lArrow->setPixmap(icon.pixmap(QSize(15, 15)));
   ui->treeWidget->setVisible(visible);
}

void RefWidget::addItems(bool isCurrentBranch, const QString& fullBranchName, const QString& sha)
{
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
         parents.append(child);
      }
      else
      {
         for (auto i = 0; i < ui->treeWidget->topLevelItemCount(); ++i)
         {
            if (ui->treeWidget->topLevelItem(i)->text(0) == folder)
            {
               child = ui->treeWidget->topLevelItem(i);
               parents.append(child);
            }
         }
      }

      if (!child)
      {
         const auto item = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem();
         item->setText(0, folder);

         if (!parent)
            ui->treeWidget->addTopLevelItem(item);

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
         ui->treeWidget->setCurrentItem(item);
         ui->treeWidget->expandItem(parent);
         const auto indexToScroll = ui->treeWidget->currentIndex();
         ui->treeWidget->scrollTo(indexToScroll);
      }
   }

   parents.clear();
   parents.squeeze();

   ui->treeWidget->addTopLevelItem(item);
}

void RefWidget::onHeaderClicked()
{
   const auto localAreVisible = ui->treeWidget->isVisible();
   const auto icon = QIcon(localAreVisible ? QString(":/icons/add") : QString(":/icons/remove"));
   ui->lArrow->setPixmap(icon.pixmap(QSize(15, 15)));
   ui->treeWidget->setVisible(!localAreVisible);

   mSettings.setLocalValue(mSettingsKey, !localAreVisible);
}
