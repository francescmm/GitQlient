/*
        Description: interface to git programs

        Author: Marco Costalba (C) 2005-2007

        Copyright: See COPYING file that comes with this distribution

*/

#include "RepositoryModel.h"
#include <RepositoryModelColumns.h>

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
   mGit->setDefaultModel(this);

   mColumns.insert(RepositoryModelColumns::GRAPH, "Graph");
   mColumns.insert(RepositoryModelColumns::ID, "Id");
   mColumns.insert(RepositoryModelColumns::SHA, "Sha");
   mColumns.insert(RepositoryModelColumns::LOG, "Log");
   mColumns.insert(RepositoryModelColumns::AUTHOR, "Author");
   mColumns.insert(RepositoryModelColumns::DATE, "Date");

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

const Rev *RepositoryModel::revLookup(int row) const
{
   const auto shaStr = sha(row);
   return !shaStr.isEmpty() ? revs.value(shaStr) : nullptr;
}

const Rev *RepositoryModel::revLookup(const QString &sha) const
{
   return !sha.isEmpty() ? revs.value(sha) : nullptr;
}

const QString RepositoryModel::getShortLog(const QString &sha)
{
   const auto r = revLookup(sha);
   return r ? r->shortLog() : QString();
}

int RepositoryModel::row(const QString &sha) const
{
   return !sha.isEmpty() && revs.value(sha) ? revs.value(sha)->orderIdx : -1;
}

QString RepositoryModel::sha(int row) const
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

   mGit->cancelDataLoading();

   beginResetModel();
   qDeleteAll(revs);
   revs.clear();
   revOrder.clear();
   firstFreeLane = loadTime = earlyOutputCntBase = 0;
   setEarlyOutputState(false);
   lns->clear();
   fNames.clear();
   curFNames.clear();

   rowCnt = revOrder.count();
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
   if (rowCnt == revOrder.count())
      return;

   beginInsertRows(QModelIndex(), rowCnt, revOrder.count() - 1);
   rowCnt = revOrder.count();
   endInsertRows();
}

void RepositoryModel::on_loadCompleted(const QString &)
{
   if (rowCnt >= revOrder.count())
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

   const auto r = revLookup(index.row());

   if (!r)
      return no_value;

   if (role == Qt::ToolTipRole)
   {
      QDateTime d;
      d.setSecsSinceEpoch(r->authorDate().toUInt());

      return QString("<p><b>Author:</b> %1</p><p><b>Date:</b> %2</p>")
          .arg(r->author().split("<").first(), d.toString(Qt::SystemLocaleShortDate));
   }

   int col = index.column();

   // calculate lanes
   if (r->lanes.count() == 0)
      mGit->setLane(r->sha());

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
      case RepositoryModelColumns::DATE:
      {
         QDateTime dt;
         dt.setSecsSinceEpoch(r->authorDate().toUInt());
         return dt.toString("dd/MM/yyyy hh:mm");
      }
      default:
         return no_value;
   }
}
