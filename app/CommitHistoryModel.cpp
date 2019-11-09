#include "CommitHistoryModel.h"

#include <CommitHistoryColumns.h>
#include <RevisionsCache.h>
#include <git.h>

#include <QDateTime>

CommitHistoryModel::CommitHistoryModel(QSharedPointer<RevisionsCache> revCache, QSharedPointer<Git> git, QObject *p)
   : QAbstractItemModel(p)
   , mRevCache(revCache)
   , mGit(git)
{
   mColumns.insert(CommitHistoryColumns::GRAPH, "Graph");
   mColumns.insert(CommitHistoryColumns::ID, "Id");
   mColumns.insert(CommitHistoryColumns::SHA, "Sha");
   mColumns.insert(CommitHistoryColumns::LOG, "Log");
   mColumns.insert(CommitHistoryColumns::AUTHOR, "Author");
   mColumns.insert(CommitHistoryColumns::DATE, "Date");

   clear();

   connect(mRevCache.get(), &RevisionsCache::signalCacheUpdated, this, &CommitHistoryModel::onNewRevisions);
}

CommitHistoryModel::~CommitHistoryModel()
{
   clear();
}

int CommitHistoryModel::rowCount(const QModelIndex &parent) const
{
   return !parent.isValid() ? rowCnt : 0;
}

bool CommitHistoryModel::hasChildren(const QModelIndex &parent) const
{

   return !parent.isValid();
}

QString CommitHistoryModel::sha(int row) const
{
   return mRevCache->sha(row);
}

void CommitHistoryModel::clear()
{
   beginResetModel();
   curFNames.clear();
   rowCnt = mRevCache->count();
   endResetModel();
   emit headerDataChanged(Qt::Horizontal, 0, 5);
}

void CommitHistoryModel::onNewRevisions()
{
   // do not process revisions if there are possible renamed points
   // or pending renamed patch to apply
   if (!renamedRevs.isEmpty() || !renamedPatches.isEmpty())
      return;

   // do not attempt to insert 0 rows since the inclusive range would be invalid
   const auto revisionsCount = mRevCache->count();
   if (rowCnt == revisionsCount)
      return;

   beginInsertRows(QModelIndex(), rowCnt, revisionsCount - 1);
   rowCnt = revisionsCount;
   endInsertRows();
}

QVariant CommitHistoryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
   if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
      return mColumns.value(static_cast<CommitHistoryColumns>(section));

   return QVariant();
}

QModelIndex CommitHistoryModel::index(int row, int column, const QModelIndex &) const
{
   if (row < 0 || row >= rowCnt)
      return QModelIndex();

   return createIndex(row, column, nullptr);
}

QModelIndex CommitHistoryModel::parent(const QModelIndex &) const
{
   return QModelIndex();
}

QVariant CommitHistoryModel::getToolTipData(const Revision &r) const
{
   QString auxMessage;
   const auto sha = r.sha();

   if ((mGit->checkRef(sha) & Git::CUR_BRANCH) && mGit->getCurrentBranchName().isEmpty())
      auxMessage.append("<p>Status: <b>detached</b></p>");

   const auto localBranches = mGit->getRefNames(sha, Git::BRANCH);

   if (!localBranches.isEmpty())
      auxMessage.append(QString("<p><b>Local: </b>%1</p>").arg(localBranches.join(",")));

   const auto remoteBranches = mGit->getRefNames(sha, Git::RMT_BRANCH);

   if (!remoteBranches.isEmpty())
      auxMessage.append(QString("<p><b>Remote: </b>%1</p>").arg(remoteBranches.join(",")));

   const auto tags = mGit->getRefNames(sha, Git::TAG);

   if (!tags.isEmpty())
      auxMessage.append(QString("<p><b>Tags: </b>%1</p>").arg(tags.join(",")));

   QDateTime d;
   d.setSecsSinceEpoch(r.authorDate().toUInt());

   return sha == ZERO_SHA
       ? QString()
       : QString("<p>%1 - %2<p></p>%3</p>%4")
             .arg(r.author().split("<").first(), d.toString(Qt::SystemLocaleShortDate), sha, auxMessage);
}

QVariant CommitHistoryModel::getDisplayData(const Revision &rev, int column) const
{
   switch (static_cast<CommitHistoryColumns>(column))
   {
      case CommitHistoryColumns::ID:
         return QVariant();
      case CommitHistoryColumns::SHA:
         return rev.sha();
      case CommitHistoryColumns::LOG:
         return rev.shortLog();
      case CommitHistoryColumns::AUTHOR:
         return rev.author().split("<").first();
      case CommitHistoryColumns::DATE: {
         QDateTime dt = QDateTime::fromSecsSinceEpoch(rev.authorDate().toUInt());
         return dt.toString("dd/MM/yyyy hh:mm");
      }
      default:
         return QVariant();
   }
}

QVariant CommitHistoryModel::data(const QModelIndex &index, int role) const
{
   if (!index.isValid() || (role != Qt::DisplayRole && role != Qt::ToolTipRole))
      return QVariant();

   const auto r = mRevCache->revLookup(index.row());
   const auto sha = r.sha();

   if (role == Qt::ToolTipRole)
      return getToolTipData(r);

   if (role == Qt::DisplayRole)
      return getDisplayData(r, index.column());

   return QVariant();
}
