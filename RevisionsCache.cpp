#include "RevisionsCache.h"

#include <Revision.h>

RevisionsCache::RevisionsCache(QSharedPointer<Git> git, QObject *parent)
   : QObject(parent)
   , mGit(git)
{
   revs.reserve(MAX_DICT_SIZE);
}

QString RevisionsCache::sha(int row) const
{
   return row >= 0 && row < revOrder.count() ? QString(revOrder.at(row)) : QString();
}

const Revision *RevisionsCache::revLookup(int row) const
{
   const auto shaStr = sha(row);
   return !shaStr.isEmpty() ? revs.value(shaStr) : nullptr;
}

const Revision *RevisionsCache::revLookup(const QString &sha) const
{
   return !sha.isEmpty() ? revs.value(sha) : nullptr;
}

void RevisionsCache::insertRevision(const QString sha, const Revision &rev)
{
   revs.insert(sha, new Revision(rev));
   revOrder.append(sha);
}

QString RevisionsCache::getShortLog(const QString &sha) const
{
   auto r = revLookup(sha);
   return r ? r->shortLog() : QString();
}

int RevisionsCache::row(const QString &sha) const
{
   return !sha.isEmpty() && revs.value(sha) ? revs.value(sha)->orderIdx : -1;
}

void RevisionsCache::flushTail(int earlyOutputCnt, int earlyOutputCntBase)
{
   if (earlyOutputCnt < 0 || earlyOutputCnt >= count())
      return;

   auto cnt = count() - earlyOutputCnt + 1;

   while (cnt > 0)
   {
      const QString &sha = revOrder.last();
      const Revision *c = revs[sha];
      delete c;
      revs.remove(sha);
      revOrder.pop_back();
      --cnt;
   }

   // reset all lanes, will be redrawn
   for (int i = earlyOutputCntBase; i < revOrder.count(); i++)
   {
      const auto c = const_cast<Revision *>(revs[revOrder[i]]);
      c->lanes.clear();
   }
}

void RevisionsCache::clear()
{
   qDeleteAll(revs);
   revs.clear();
   revOrder.clear();
}
