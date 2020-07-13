#include "CommitHistoryView.h"

#include <CommitHistoryModel.h>
#include <CommitHistoryColumns.h>
#include <CommitHistoryContextMenu.h>
#include <ShaFilterProxyModel.h>
#include <CommitInfo.h>
#include <RevisionsCache.h>
#include <GitConfig.h>

#include <QHeaderView>
#include <QSettings>
#include <QDateTime>

#include <QLogger.h>
using namespace QLogger;

CommitHistoryView::CommitHistoryView(const QSharedPointer<RevisionsCache> &cache, const QSharedPointer<GitBase> &git,
                                     QWidget *parent)
   : QTreeView(parent)
   , mCache(cache)
   , mGit(git)
{
   setEnabled(false);
   setContextMenuPolicy(Qt::CustomContextMenu);
   setItemsExpandable(false);
   setMouseTracking(true);
   setSelectionMode(QAbstractItemView::ExtendedSelection);
   setAttribute(Qt::WA_DeleteOnClose);

   header()->setSortIndicatorShown(false);

   connect(header(), &QHeaderView::sectionResized, this, &CommitHistoryView::saveHeaderState);
   connect(mCache.get(), &RevisionsCache::signalCacheUpdated, this, [this]() {
      QModelIndex topLeft;
      QModelIndex bottomRight;

      if (mProxyModel)
      {
         topLeft = mProxyModel->index(0, 0);
         bottomRight = mProxyModel->index(mProxyModel->rowCount() - 1, mProxyModel->columnCount() - 1);
         mProxyModel->beginResetModel();
         mProxyModel->endResetModel();
      }
      else
      {
         topLeft = mCommitHistoryModel->index(0, 0);
         bottomRight
             = mCommitHistoryModel->index(mCommitHistoryModel->rowCount() - 1, mCommitHistoryModel->columnCount() - 1);
         mCommitHistoryModel->onNewRevisions(mCache->count());
      }

      const auto auxTL = visualRect(topLeft);
      const auto auxBR = visualRect(bottomRight);
      viewport()->update(auxTL.x(), auxTL.y(), auxBR.x() + auxBR.width(), auxBR.y() + auxBR.height());
   });
}

void CommitHistoryView::setModel(QAbstractItemModel *model)
{
   connect(this, &CommitHistoryView::customContextMenuRequested, this, &CommitHistoryView::showContextMenu,
           Qt::UniqueConnection);

   mCommitHistoryModel = dynamic_cast<CommitHistoryModel *>(model);
   QTreeView::setModel(model);
   setupGeometry();
   connect(this->selectionModel(), &QItemSelectionModel::selectionChanged, this,
           [this](const QItemSelection &selected, const QItemSelection &) {
              const auto indexes = selected.indexes();
              if (!indexes.isEmpty())
              {
                 scrollTo(indexes.first());
                 emit clicked(indexes.first());
              }
           });
}

void CommitHistoryView::filterBySha(const QStringList &shaList)
{
   mIsFiltering = true;

   if (mProxyModel)
   {
      mProxyModel->beginResetModel();
      mProxyModel->setAcceptedSha(shaList);
      mProxyModel->endResetModel();
   }
   else
   {
      mProxyModel = new ShaFilterProxyModel(this);
      mProxyModel->setSourceModel(mCommitHistoryModel);
      mProxyModel->setAcceptedSha(shaList);
      setModel(mProxyModel);
   }

   setupGeometry();
}

CommitHistoryView::~CommitHistoryView()
{
   QSettings s;
   s.setValue(QString("%1").arg(objectName()), header()->saveState());
}

void CommitHistoryView::setupGeometry()
{
   QSettings s;
   const auto previousState = s.value(QString("%1").arg(objectName()), QByteArray()).toByteArray();

   if (previousState.isEmpty())
   {
      const auto hv = header();
      hv->setMinimumSectionSize(75);
      hv->resizeSection(static_cast<int>(CommitHistoryColumns::SHA), 75);
      hv->resizeSection(static_cast<int>(CommitHistoryColumns::GRAPH), 120);
      hv->resizeSection(static_cast<int>(CommitHistoryColumns::AUTHOR), 160);
      hv->resizeSection(static_cast<int>(CommitHistoryColumns::DATE), 125);
      hv->resizeSection(static_cast<int>(CommitHistoryColumns::SHA), 75);
      hv->setSectionResizeMode(static_cast<int>(CommitHistoryColumns::AUTHOR), QHeaderView::Fixed);
      hv->setSectionResizeMode(static_cast<int>(CommitHistoryColumns::DATE), QHeaderView::Fixed);
      hv->setSectionResizeMode(static_cast<int>(CommitHistoryColumns::SHA), QHeaderView::Fixed);
      hv->setSectionResizeMode(static_cast<int>(CommitHistoryColumns::LOG), QHeaderView::Stretch);
      hv->setStretchLastSection(false);

      hideColumn(static_cast<int>(CommitHistoryColumns::ID));
   }
   else
   {
      header()->restoreState(previousState);
      header()->setSectionResizeMode(static_cast<int>(CommitHistoryColumns::LOG), QHeaderView::Stretch);
   }
}

void CommitHistoryView::currentChanged(const QModelIndex &index, const QModelIndex &)
{
   mCurrentSha = model()->index(index.row(), static_cast<int>(CommitHistoryColumns::SHA)).data().toString();
}

void CommitHistoryView::clear()
{
   mCommitHistoryModel->clear();
}

void CommitHistoryView::focusOnCommit(const QString &goToSha)
{
   mCurrentSha = goToSha;

   QLog_Info("UI", QString("Setting the focus on the commit {%1}").arg(mCurrentSha));

   auto row = mCache->getCommitPos(mCurrentSha);

   if (mIsFiltering)
   {
      const auto sourceIndex = mProxyModel->sourceModel()->index(row, 0);
      row = mProxyModel->mapFromSource(sourceIndex).row();
   }

   clearSelection();

   const auto index = model()->index(row, 0);

   setCurrentIndex(index);
   scrollTo(currentIndex());
}

QModelIndexList CommitHistoryView::selectedIndexes() const
{
   return QTreeView::selectedIndexes();
}

void CommitHistoryView::showContextMenu(const QPoint &pos)
{
   if (!mIsFiltering)
   {
      const auto shas = getSelectedShaList();

      if (!shas.isEmpty())
      {
         const auto menu = new CommitHistoryContextMenu(mCache, mGit, shas, this);
         connect(menu, &CommitHistoryContextMenu::signalRefreshPRsCache, mCache.get(),
                 &RevisionsCache::refreshPRsCache);
         connect(menu, &CommitHistoryContextMenu::signalRepositoryUpdated, this, &CommitHistoryView::signalViewUpdated);
         connect(menu, &CommitHistoryContextMenu::signalOpenDiff, this, &CommitHistoryView::signalOpenDiff);
         connect(menu, &CommitHistoryContextMenu::signalOpenCompareDiff, this,
                 &CommitHistoryView::signalOpenCompareDiff);
         connect(menu, &CommitHistoryContextMenu::signalAmendCommit, this, &CommitHistoryView::signalAmendCommit);
         connect(menu, &CommitHistoryContextMenu::signalMergeRequired, this, &CommitHistoryView::signalMergeRequired);
         connect(menu, &CommitHistoryContextMenu::signalCherryPickConflict, this,
                 &CommitHistoryView::signalCherryPickConflict);
         connect(menu, &CommitHistoryContextMenu::signalPullConflict, this, &CommitHistoryView::signalPullConflict);
         menu->exec(viewport()->mapToGlobal(pos));
      }
      else
         QLog_Warning("UI", "SHAs selected belong to different branches. They need to share at least one branch.");
   }
}

void CommitHistoryView::saveHeaderState()
{
   QSettings s;
   s.setValue(QString("%1").arg(objectName()), header()->saveState());
}

QList<QString> CommitHistoryView::getSelectedShaList() const
{
   const auto indexes = selectedIndexes();
   QMap<QDateTime, QString> shas;
   QVector<QVector<QString>> godVector;

   for (auto index : indexes)
   {
      const auto sha = mCommitHistoryModel->sha(index.row());
      const auto dtStr
          = mCommitHistoryModel->index(index.row(), static_cast<int>(CommitHistoryColumns::DATE)).data().toString();
      const auto dt = QDateTime::fromString(dtStr, "dd MMM yyyy hh:mm");

      shas.insert(dt, sha);

      const auto commit = mCache->getCommitInfo(sha);
      auto branches = commit.getReferences(References::Type::LocalBranch)
          + commit.getReferences(References::Type::RemoteBranches);

      std::sort(branches.begin(), branches.end());

      if (!branches.isEmpty())
         godVector.append(branches.toVector());
   }

   auto commitsInSameBranch = false;

   if (godVector.count() > 1)
   {
      QVector<QString> common;

      for (auto i = 0; i < godVector.count() - 1; ++i)
      {
         QVector<QString> aux;
         std::set_intersection(godVector.at(i).constBegin(), godVector.at(i).constEnd(),
                               godVector.at(i + 1).constBegin(), godVector.at(i + 1).constEnd(),
                               std::back_inserter(aux));
         common = aux;
      }

      commitsInSameBranch = !common.isEmpty();
   }

   return commitsInSameBranch || shas.count() == 1 ? shas.values() : QList<QString>();
}
