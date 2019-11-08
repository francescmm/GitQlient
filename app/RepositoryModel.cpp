/*
        Description: interface to git programs

        Author: Marco Costalba (C) 2005-2007

        Copyright: See COPYING file that comes with this distribution

*/

#include "RepositoryModel.h"
#include <RepositoryModelColumns.h>
#include <RevisionsCache.h>
#include <Revision.h>

#include <QApplication>
#include <QDateTime>
#include <QFontMetrics>

#include "lanes.h"

#include "git.h"

RepositoryModel::RepositoryModel(QSharedPointer<RevisionsCache> revCache, QSharedPointer<Git> git, QObject *p)
   : QAbstractItemModel(p)
   , mRevCache(revCache)
   , mGit(git)
{
   mGit->setDefaultModel(this);

   mColumns.insert(RepositoryModelColumns::GRAPH, "Graph");
   mColumns.insert(RepositoryModelColumns::ID, "Id");
   mColumns.insert(RepositoryModelColumns::SHA, "Sha");
   mColumns.insert(RepositoryModelColumns::LOG, "Log");
   mColumns.insert(RepositoryModelColumns::AUTHOR, "Author");
   mColumns.insert(RepositoryModelColumns::DATE, "Date");

   clear(); // after _headerInfo is set

   connect(mGit.get(), &Git::newRevsAdded, this, &RepositoryModel::on_newRevsAdded);
   connect(mGit.get(), &Git::loadCompleted, this, &RepositoryModel::on_loadCompleted);
   connect(mRevCache.get(), &RevisionsCache::signalCacheUpdated, this, &RepositoryModel::on_newRevsAdded);
}

RepositoryModel::~RepositoryModel()
{
   clear();
}

int RepositoryModel::rowCount(const QModelIndex &parent) const
{
   return !parent.isValid() ? rowCnt : 0;
}

bool RepositoryModel::hasChildren(const QModelIndex &parent) const
{

   return !parent.isValid();
}

QString RepositoryModel::sha(int row) const
{
   return mRevCache->sha(row);
}

void RepositoryModel::flushTail()
{
   beginResetModel();
   mRevCache->flushTail(earlyOutputCnt, earlyOutputCntBase);
   // lns->clear();
   rowCnt = mRevCache->count();
   endResetModel();
}

void RepositoryModel::clear(bool complete)
{

   if (!complete)
   {
      if (!mRevCache->isEmpty())
         flushTail();

      return;
   }

   mGit->cancelDataLoading();

   beginResetModel();

   mRevCache->clear();

   setEarlyOutputState(false);
   // lns->clear();
   curFNames.clear();

   rowCnt = mRevCache->count();
   endResetModel();
   emit headerDataChanged(Qt::Horizontal, 0, 5);
}

void RepositoryModel::on_newRevsAdded()
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

void RepositoryModel::on_loadCompleted()
{
   const auto revisionsCount = mRevCache->count();
   if (rowCnt >= revisionsCount)
      return;

   // now we can process last revision
   rowCnt = revisionsCount;
   beginResetModel(); // force a reset to avoid artifacts in file history graph under Windows
   endResetModel();
}

QVariant RepositoryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
   if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
      return mColumns.value(static_cast<RepositoryModelColumns>(section));

   return QVariant();
}

QModelIndex RepositoryModel::index(int row, int column, const QModelIndex &) const
{
   if (row < 0 || row >= rowCnt)
      return QModelIndex();

   return createIndex(row, column, nullptr);
}

QModelIndex RepositoryModel::parent(const QModelIndex &) const
{
   return QModelIndex();
}

QVariant RepositoryModel::getToolTipData(const Revision &r) const
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

   return QString("<p>%1 - %2<p></p>%3</p>%4")
       .arg(r.author().split("<").first(), d.toString(Qt::SystemLocaleShortDate), sha, auxMessage);
}

QVariant RepositoryModel::getDisplayData(const Revision &rev, int column) const
{
   switch (static_cast<RepositoryModelColumns>(column))
   {
      case RepositoryModelColumns::ID:
         return QVariant();
      case RepositoryModelColumns::SHA:
         return rev.sha();
      case RepositoryModelColumns::LOG:
         return rev.shortLog();
      case RepositoryModelColumns::AUTHOR:
         return rev.author().split("<").first();
      case RepositoryModelColumns::DATE: {
         QDateTime dt = QDateTime::fromSecsSinceEpoch(rev.authorDate().toUInt());
         return dt.toString("dd/MM/yyyy hh:mm");
      }
      default:
         return QVariant();
   }
}

QVariant RepositoryModel::data(const QModelIndex &index, int role) const
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
