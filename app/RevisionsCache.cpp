#include "RevisionsCache.h"

RevisionsCache::RevisionsCache(QSharedPointer<Git> git, QObject *parent)
   : QObject(parent)
   , mGit(git)
{
}

QString RevisionsCache::sha(int row) const
{
   return row >= 0 && row < revOrder.count() ? QString(revOrder.at(row)) : QString();
}

Revision RevisionsCache::revLookup(int row) const
{
   const auto shaStr = sha(row);
   return revs.value(shaStr, Revision());
}

Revision RevisionsCache::getRevLookup(const QString &sha) const
{
   if (!sha.isEmpty())
   {
      const auto iter = std::find_if(revs.constBegin(), revs.constEnd(),
                                     [sha](const Revision &revision) { return revision.sha().startsWith(sha); });

      if (iter != std::end(revs))
         return *iter;
   }

   return Revision();
}

void RevisionsCache::insertRevision(const QString sha, const Revision &rev)
{
   auto r = rev;

   if (r.lanes.count() == 0)
      updateLanes(r, lns);

   revs.insert(sha, r);

   if (!revOrder.contains(sha))
      revOrder.append(sha);
}

void RevisionsCache::updateLanes(Revision &c, Lanes &lns)
{
   const auto sha = c.sha();

   if (lns.isEmpty())
      lns.init(c.sha());

   bool isDiscontinuity;
   bool isFork = lns.isFork(sha, isDiscontinuity);
   bool isMerge = (c.parentsCount() > 1);
   bool isInitial = (c.parentsCount() == 0);

   if (isDiscontinuity)
      lns.changeActiveLane(sha); // uses previous isBoundary state

   lns.setBoundary(c.isBoundary()); // update must be here

   if (isFork)
      lns.setFork(sha);
   if (isMerge)
      lns.setMerge(c.parents());
   if (c.isApplied)
      lns.setApplied();
   if (isInitial)
      lns.setInitial();

   lns.setLanes(c.lanes); // here lanes are snapshotted

   const QString &nextSha = (isInitial) ? "" : QString(c.parent(0));

   lns.nextParent(nextSha);

   if (c.isApplied)
      lns.afterApplied();
   if (isMerge)
      lns.afterMerge();
   if (isFork)
      lns.afterFork();
   if (lns.isBranch())
      lns.afterBranch();

   // lns.setLanes(c.lanes); // here lanes are snapshotted
}

QString RevisionsCache::getShortLog(const QString &sha) const
{
   return getRevLookup(sha).shortLog();
}

int RevisionsCache::row(const QString &sha) const
{
   return revs.value(sha).orderIdx;
}

void RevisionsCache::flushTail(int earlyOutputCnt, int earlyOutputCntBase)
{
   if (earlyOutputCnt < 0 || earlyOutputCnt >= count())
      return;

   auto cnt = count() - earlyOutputCnt + 1;

   while (cnt > 0)
   {
      const auto sha = revOrder.last();
      revs.remove(sha);
      revOrder.pop_back();
      --cnt;
   }

   // reset all lanes, will be redrawn
   for (int i = earlyOutputCntBase; i < revOrder.count(); i++)
      revs[revOrder[i]].lanes.clear();
}

void RevisionsCache::clear()
{
   revs.clear();
   revOrder.clear();
}
