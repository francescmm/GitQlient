/*
        Description: interface to git programs

        Author: Marco Costalba (C) 2005-2007

        Copyright: See COPYING file that comes with this distribution

*/

#include "RepositoryModel.h"

#include <QApplication>
#include <QDateTime>
#include <QFontMetrics>

#include "lanes.h"

#include "git.h"

using namespace QGit;

RepositoryModel::RepositoryModel(QSharedPointer<Git> git, QObject *p)
   : QAbstractItemModel(p)
   , mGit(git)
{
   mColumns.insert(FileHistoryColumn::GRAPH, "Graph");
   mColumns.insert(FileHistoryColumn::ID, "Id");
   mColumns.insert(FileHistoryColumn::SHA, "Sha");
   mColumns.insert(FileHistoryColumn::LOG, "Log");
   mColumns.insert(FileHistoryColumn::AUTHOR, "Author");
   mColumns.insert(FileHistoryColumn::DATE, "Date");

   lns = new Lanes();
   revs.reserve(QGit::MAX_DICT_SIZE);
   clear(); // after _headerInfo is set

   connect(mGit.get(), &Git::newRevsAdded, this, &RepositoryModel::on_newRevsAdded);
   connect(mGit.get(), &Git::loadCompleted, this, &RepositoryModel::on_loadCompleted);
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

int RepositoryModel::row(const QString &sha) const
{

   const Rev *r = mGit->revLookup(sha, this);
   return r ? r->orderIdx : -1;
}

const QString RepositoryModel::sha(int row) const
{

   return row >= 0 && row < rowCnt ? QString(revOrder.at(row)) : "";
}

void RepositoryModel::flushTail()
{

   if (earlyOutputCnt < 0 || earlyOutputCnt >= revOrder.count())
      return;

   int cnt = revOrder.count() - earlyOutputCnt + 1;
   beginResetModel();
   while (cnt > 0)
   {
      const QString &sha = revOrder.last();
      const Rev *c = revs[sha];
      delete c;
      revs.remove(sha);
      revOrder.pop_back();
      cnt--;
   }
   // reset all lanes, will be redrawn
   for (int i = earlyOutputCntBase; i < revOrder.count(); i++)
   {
      const auto c = const_cast<Rev *>(revs[revOrder[i]]);
      c->lanes.clear();
   }
   firstFreeLane = static_cast<unsigned int>(earlyOutputCntBase);
   lns->clear();
   rowCnt = revOrder.count();
   endResetModel();
}

void RepositoryModel::clear(bool complete)
{

   if (!complete)
   {
      if (!revOrder.isEmpty())
         flushTail();

      return;
   }

   mGit->cancelDataLoading(this);

   beginResetModel();
   qDeleteAll(revs);
   revs.clear();
   revOrder.clear();
   firstFreeLane = loadTime = earlyOutputCntBase = 0;
   setEarlyOutputState(false);
   lns->clear();
   fNames.clear();
   curFNames.clear();
   qDeleteAll(rowData);
   rowData.clear();

   rowCnt = revOrder.count();
   annIdValid = false;
   endResetModel();
   emit headerDataChanged(Qt::Horizontal, 0, 5);
}

void RepositoryModel::on_newRevsAdded(const RepositoryModel *fh, const QVector<QString> &shaVec)
{

   if (fh != this) // signal newRevsAdded() is broadcast
      return;

   // do not process revisions if there are possible renamed points
   // or pending renamed patch to apply
   if (!renamedRevs.isEmpty() || !renamedPatches.isEmpty())
      return;

   // do not attempt to insert 0 rows since the inclusive range would be invalid
   if (rowCnt == shaVec.count())
      return;

   beginInsertRows(QModelIndex(), rowCnt, shaVec.count() - 1);
   rowCnt = shaVec.count();
   endInsertRows();
}

void RepositoryModel::on_loadCompleted(const RepositoryModel *fh, const QString &)
{

   if (fh != this || rowCnt >= revOrder.count())
      return;

   // now we can process last revision
   rowCnt = revOrder.count();
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

   mColumns[FileHistoryColumn::ID] = id;
   emit headerDataChanged(Qt::Horizontal, 1, 1);
}

QVariant RepositoryModel::headerData(int section, Qt::Orientation orientation, int role) const
{

   if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
      return mColumns.value(static_cast<FileHistoryColumn>(section));

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

const QString RepositoryModel::timeDiff(unsigned long secs) const
{

   ulong days = secs / (3600 * 24);
   ulong hours = (secs - days * 3600 * 24) / 3600;
   ulong min = (secs - days * 3600 * 24 - hours * 3600) / 60;
   ulong sec = secs - days * 3600 * 24 - hours * 3600 - min * 60;
   QString tmp;
   if (days > 0)
      tmp.append(QString::number(days) + "d ");

   if (hours > 0 || !tmp.isEmpty())
      tmp.append(QString::number(hours) + "h ");

   if (min > 0 || !tmp.isEmpty())
      tmp.append(QString::number(min) + "m ");

   tmp.append(QString::number(sec) + "s");
   return tmp;
}

QVariant RepositoryModel::data(const QModelIndex &index, int role) const
{

   static const QVariant no_value;

   if (!index.isValid() || (role != Qt::DisplayRole && role != Qt::ToolTipRole))
      return no_value; // fast path, 90% of calls ends here!

   const auto git = mGit;
   const Rev *r = git->revLookup(revOrder.at(index.row()), this);
   if (!r)
      return no_value;

   if (role == Qt::ToolTipRole)
   {
      return QString("<p><b>Author:</b> %1</p><p><b>Date:</b> %2</p>")
          .arg(r->author().split("<").first(), git->getLocalDate(r->authorDate()));
   }

   int col = index.column();

   // calculate lanes
   if (r->lanes.count() == 0)
      git->setLane(r->sha(), const_cast<RepositoryModel *>(this));

   switch (static_cast<FileHistoryColumn>(col))
   {
      case FileHistoryColumn::ID:
         return (annIdValid ? rowCnt - index.row() : QVariant());
      case FileHistoryColumn::SHA:
         return r->sha();
      case FileHistoryColumn::LOG:
         return r->shortLog();
      case FileHistoryColumn::AUTHOR:
         return r->author().split("<").first();
      case FileHistoryColumn::DATE:
      {
         QDateTime dt;
         dt.setSecsSinceEpoch(r->authorDate().toUInt());
         return dt.toString("dd/MM/yyyy hh:mm");
      }
      default:
         return no_value;
   }
}
