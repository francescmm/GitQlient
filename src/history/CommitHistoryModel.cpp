#include "CommitHistoryModel.h"

#include <CommitHistoryColumns.h>
#include <CommitInfo.h>
#include <GitCache.h>
#include <GitServerCache.h>
#include <GitBase.h>

#include <QDateTime>
#include <QLocale>

CommitHistoryModel::CommitHistoryModel(const QSharedPointer<GitCache> &cache, const QSharedPointer<GitBase> &git,
                                       const QSharedPointer<GitServerCache> &gitServerCache, QObject *p)
   : QAbstractItemModel(p)
   , mCache(cache)
   , mGit(git)
   , mGitServerCache(gitServerCache)
{
   mColumns.insert(CommitHistoryColumns::TreeViewIcon, "");
   mColumns.insert(CommitHistoryColumns::Graph, "Graph");
   mColumns.insert(CommitHistoryColumns::Sha, "Sha");
   mColumns.insert(CommitHistoryColumns::Log, "Log");
   mColumns.insert(CommitHistoryColumns::Author, "Author");
   mColumns.insert(CommitHistoryColumns::Date, "Date");
}

int CommitHistoryModel::rowCount(const QModelIndex &parent) const
{
   return !parent.isValid() ? mCache->count() : 0;
}

bool CommitHistoryModel::hasChildren(const QModelIndex &parent) const
{
   return !parent.isValid();
}

QString CommitHistoryModel::sha(int row) const
{
   return index(row, static_cast<int>(CommitHistoryColumns::Sha)).data().toString();
}

void CommitHistoryModel::clear()
{
   beginResetModel();
   endResetModel();
   emit headerDataChanged(Qt::Horizontal, 0, 5);
}

void CommitHistoryModel::onNewRevisions(int totalCommits)
{
   beginResetModel();
   endResetModel();

   beginInsertRows(QModelIndex(), 0, totalCommits - 2);
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
   return row >= 0 && row < mCache->count() ? createIndex(row, column, nullptr) : QModelIndex();
}

QModelIndex CommitHistoryModel::parent(const QModelIndex &) const
{
   return QModelIndex();
}

QVariant CommitHistoryModel::getToolTipData(const CommitInfo &r) const
{
   QString auxMessage;
   const auto sha = r.sha();

   if (mGit->getCurrentBranch().isEmpty())
      auxMessage.append("<p>Status: <b>detached</b></p>");

   const auto localBranches = r.getReferences(References::Type::LocalBranch);

   if (!localBranches.isEmpty())
      auxMessage.append(QString("<p><b>Local: </b>%1</p>").arg(localBranches.join(",")));

   const auto remoteBranches = r.getReferences(References::Type::RemoteBranches);

   if (!remoteBranches.isEmpty())
      auxMessage.append(QString("<p><b>Remote: </b>%1</p>").arg(remoteBranches.join(",")));

   const auto tags = r.getReferences(References::Type::LocalTag);

   if (!tags.isEmpty())
      auxMessage.append(QString("<p><b>Tags: </b>%1</p>").arg(tags.join(",")));

   QDateTime d;
   d.setSecsSinceEpoch(r.authorDate().toUInt());

   QLocale locale;

   auto tooltip = sha == CommitInfo::ZERO_SHA
       ? QString()
       : QString("<p>%1 - %2</p><p>%3</p>%4%5")
             .arg(r.author().split("<").first(), d.toString(locale.dateTimeFormat(QLocale::ShortFormat)), sha,
                  !auxMessage.isEmpty() ? QString("<p>%1</p>").arg(auxMessage) : "",
                  r.isSigned() ? QString::fromUtf8("<p>Commit signed!</p><p> GPG key: %1</p>").arg(r.getGpgKey()) : "");

   if (mGitServerCache)
   {
      if (const auto pr = mGitServerCache->getPullRequest(sha); pr.isValid())
         tooltip.append(QString("<p><b>PR state: </b>%1.</p>").arg(pr.state.state));
   }

   return tooltip;
}

QVariant CommitHistoryModel::getDisplayData(const CommitInfo &rev, int column) const
{
   switch (static_cast<CommitHistoryColumns>(column))
   {
      case CommitHistoryColumns::Sha: {
         const auto sha = rev.sha();
         return sha;
      }
      case CommitHistoryColumns::Log:
         return rev.shortLog();
      case CommitHistoryColumns::Author: {
         const auto author = rev.author().split("<").first();
         return author;
      }
      case CommitHistoryColumns::Date: {
         return QDateTime::fromSecsSinceEpoch(rev.authorDate().toUInt()).toString("dd MMM yyyy hh:mm");
      }
      default:
         return QVariant();
   }
}

QVariant CommitHistoryModel::data(const QModelIndex &index, int role) const
{
   if (!index.isValid() || (role != Qt::DisplayRole && role != Qt::ToolTipRole))
      return QVariant();

   const auto r = mCache->getCommitInfoByRow(index.row());

   if (role == Qt::ToolTipRole)
      return getToolTipData(r);

   if (role == Qt::DisplayRole)
      return getDisplayData(r, index.column());

   return QVariant();
}
