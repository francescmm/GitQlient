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

   lns = new Lanes();
   clear(); // after _headerInfo is set

   connect(mGit.get(), &Git::newRevsAdded, this, &RepositoryModel::on_newRevsAdded);
   connect(mGit.get(), &Git::loadCompleted, this, &RepositoryModel::on_loadCompleted);
   connect(mRevCache.get(), &RevisionsCache::signalCacheUpdated, this, &RepositoryModel::on_newRevsAdded);
}

RepositoryModel::~RepositoryModel()
{

   clear();
   delete lns;
}

void RepositoryModel::resetFileNames(const QString &fn)
{

   fNames.clear();
   fNames.append(fn);
   curFNames = fNames;
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
   firstFreeLane = static_cast<unsigned int>(earlyOutputCntBase);
   lns->clear();
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

   firstFreeLane = loadTime = earlyOutputCntBase = 0;
   setEarlyOutputState(false);
   lns->clear();
   fNames.clear();
   curFNames.clear();

   rowCnt = mRevCache->count();
   annIdValid = false;
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

void RepositoryModel::on_loadCompleted(const QString &)
{
   const auto revisionsCount = mRevCache->count();
   if (rowCnt >= revisionsCount)
      return;

   // now we can process last revision
   rowCnt = revisionsCount;
   beginResetModel(); // force a reset to avoid artifacts in file history graph under Windows
   endResetModel();
}

void RepositoryModel::on_changeFont(const QFont &f)
{

   QString maxStr(QString::number(rowCnt).length() + 1, '8');
   QFontMetrics fmRows(f);
   int neededWidth = fmRows.boundingRect(maxStr).width();

   QString id("Id");
   QFontMetrics fmId(qApp->font());

   while (fmId.boundingRect(id).width() < neededWidth)
      id += ' ';

   mColumns[RepositoryModelColumns::ID] = id;
   emit headerDataChanged(Qt::Horizontal, 1, 1);
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

   static const QModelIndex no_parent;
   return no_parent;
}

QVariant RepositoryModel::data(const QModelIndex &index, int role) const
{

   static const QVariant no_value;

   if (!index.isValid() || (role != Qt::DisplayRole && role != Qt::ToolTipRole))
      return no_value; // fast path, 90% of calls ends here!

   const auto r = mRevCache->revLookup(index.row());
   if (!r)
      return no_value;

   const auto sha = r->sha();

   if (role == Qt::ToolTipRole)
   {
      if (sha != ZERO_SHA)
      {
         QString auxMessage;

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
         d.setSecsSinceEpoch(r->authorDate().toUInt());

         return QString("<p>%1 - %2<p></p>%3</p>%4")
             .arg(r->author().split("<").first(), d.toString(Qt::SystemLocaleShortDate), sha, auxMessage);
      }
      else
         return QString("WIP: Local changes");
   }

   int col = index.column();

   // calculate lanes
   if (r->lanes.count() == 0)
      mGit->setLane(sha);

   switch (static_cast<RepositoryModelColumns>(col))
   {
      case RepositoryModelColumns::ID:
         return (annIdValid ? rowCnt - index.row() : QVariant());
      case RepositoryModelColumns::SHA:
         return r->sha();
      case RepositoryModelColumns::LOG:
         return r->shortLog();
      case RepositoryModelColumns::AUTHOR:
         return r->author().split("<").first();
      case RepositoryModelColumns::DATE: {
         QDateTime dt;
         dt.setSecsSinceEpoch(r->authorDate().toUInt());
         return dt.toString("dd/MM/yyyy hh:mm");
      }
      default:
         return no_value;
   }
}
