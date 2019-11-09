#include "RevisionsCache.h"

RevisionsCache::RevisionsCache(QObject *parent)
   : QObject(parent)
{
}

QString RevisionsCache::sha(int row) const
{
   return row >= 0 && row < revOrder.count() ? QString(revOrder.at(row)) : QString();
}

Revision RevisionsCache::getRevLookupByRow(int row) const
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

void RevisionsCache::insertRevision(const Revision &rev)
{
   const auto sha = rev.sha();

   if (!revs.contains(sha))
   {
      auto r = rev;

      if (r.lanes.count() == 0)
         updateLanes(r, lns);

      revs.insert(sha, r);

      if (revs.contains(rev.parent(0)))
         revs.remove(rev.parent(0));
   }

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

void RevisionsCache::clear()
{
   lns.clear();
   // revs.clear();
   revOrder.clear();
}

QString RevisionsCache::getLaneParent(const QString &fromSHA, int laneNum)
{
   const auto rs = getRevLookup(fromSHA);
   if (rs.sha().isEmpty())
      return "";

   for (int idx = rs.orderIdx - 1; idx >= 0; idx--)
   {

      const auto r = getRevLookup(getRevisionSha(idx));
      if (laneNum < r.lanes.count())
      {
         auto type = r.lanes.at(laneNum);

         if (!isFreeLane(type))
         {
            auto parNum = 0;

            while (!isMerge(type) && type != LaneType::ACTIVE)
            {

               if (isHead(static_cast<LaneType>(type)))
                  parNum++;

               type = r.lanes[--laneNum];
            }
            return r.parent(parNum);
         }
      }
   }
   return QString();
}
