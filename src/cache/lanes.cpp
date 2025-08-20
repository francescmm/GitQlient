/*
        Description: history graph computation

        Author: Marco Costalba (C) 2005-2007

        Copyright: See COPYING file that comes with this distribution

*/
#include <lanes.h>

#include <QStringList>

void Lanes::init(const QString &expectedSha)
{
   clear();
   activeLane = 0;
   add(LaneType::BRANCH, expectedSha, activeLane);
}

void Lanes::clear()
{
   laneTypes.clear();
   shaTracker.clear();
}

bool Lanes::isFork(const QString &sha, bool &isDiscontinuity)
{
   int pos = shaTracker.findNextSha(sha, 0);
   isDiscontinuity = activeLane != pos;
   return pos == -1 ? false : shaTracker.findNextSha(sha, pos + 1) != -1;
}

void Lanes::setFork(const QString &sha)
{
   auto rangeEnd = 0;
   auto idx = 0;
   auto rangeStart = rangeEnd = idx = shaTracker.findNextSha(sha, 0);

   while (idx != -1)
   {
      rangeEnd = idx;
      laneTypes.setType(idx, LaneType::TAIL);
      idx = shaTracker.findNextSha(sha, idx + 1);
   }

   laneTypes.setType(activeLane, LaneType::MERGE_FORK);

   auto &startT = laneTypes[rangeStart];
   auto &endT = laneTypes[rangeEnd];

   if (startT.equals(LaneType::MERGE_FORK))
      startT.setType(LaneType::MERGE_FORK_L);

   if (endT.equals(LaneType::MERGE_FORK))
      endT.setType(LaneType::MERGE_FORK_R);

   if (startT.equals(LaneType::TAIL))
      startT.setType(LaneType::TAIL_L);

   if (endT.equals(LaneType::TAIL))
      endT.setType(LaneType::TAIL_R);

   for (int i = rangeStart + 1; i < rangeEnd; ++i)
   {
      switch (auto &t = laneTypes[i]; t.getType())
      {
         case LaneType::NOT_ACTIVE:
            t.setType(LaneType::CROSS);
            break;
         case LaneType::EMPTY:
            t.setType(LaneType::CROSS_EMPTY);
            break;
         default:
            break;
      }
   }
}

void Lanes::setMerge(const QStringList &parents)
{
   auto &t = laneTypes[activeLane];
   auto wasFork = t.equals(LaneType::MERGE_FORK);
   auto wasFork_L = t.equals(LaneType::MERGE_FORK_L);
   auto wasFork_R = t.equals(LaneType::MERGE_FORK_R);
   auto startJoinWasACross = false;
   auto endJoinWasACross = false;

   t.setType(LaneType::MERGE_FORK);

   auto rangeStart = activeLane;
   auto rangeEnd = activeLane;
   QStringList::const_iterator it(parents.constBegin());

   for (++it; it != parents.constEnd(); ++it)
   {
      int idx = shaTracker.findNextSha(*it, 0);

      if (idx != -1)
      {
         if (idx > rangeEnd)
         {
            rangeEnd = idx;
            endJoinWasACross = laneTypes[idx].equals(LaneType::CROSS);
         }

         if (idx < rangeStart)
         {
            rangeStart = idx;
            startJoinWasACross = laneTypes[idx].equals(LaneType::CROSS);
         }

         laneTypes.setType(idx, LaneType::JOIN);
      }
      else
         rangeEnd = add(LaneType::HEAD, *it, rangeEnd + 1);
   }

   auto &startT = laneTypes[rangeStart];
   auto &endT = laneTypes[rangeEnd];

   if (startT.equals(LaneType::MERGE_FORK) && !wasFork && !wasFork_R)
      startT.setType(LaneType::MERGE_FORK_L);

   if (endT.equals(LaneType::MERGE_FORK) && !wasFork && !wasFork_L)
      endT.setType(LaneType::MERGE_FORK_R);

   if (startT.equals(LaneType::JOIN) && !startJoinWasACross)
      startT.setType(LaneType::JOIN_L);

   if (endT.equals(LaneType::JOIN) && !endJoinWasACross)
      endT.setType(LaneType::JOIN_R);

   if (startT.equals(LaneType::HEAD))
      startT.setType(LaneType::HEAD_L);

   if (endT.equals(LaneType::HEAD))
      endT.setType(LaneType::HEAD_R);

   for (int i = rangeStart + 1; i < rangeEnd; i++)
   {
      auto &t = laneTypes[i];

      if (t.equals(LaneType::NOT_ACTIVE))
         t.setType(LaneType::CROSS);
      else if (t.equals(LaneType::EMPTY))
         t.setType(LaneType::CROSS_EMPTY);
      else if (t.equals(LaneType::TAIL_R) || t.equals(LaneType::TAIL_L))
         t.setType(LaneType::TAIL);
   }
}

void Lanes::setInitial()
{
   auto &t = laneTypes[activeLane];
   if (!laneTypes.isNode(t))
      t.setType(LaneType::INITIAL);
}

void Lanes::changeActiveLane(const QString &sha)
{
   auto &t = laneTypes[activeLane];

   if (t.equals(LaneType::INITIAL))
      t.setType(LaneType::EMPTY);
   else
      t.setType(LaneType::NOT_ACTIVE);

   int idx = shaTracker.findNextSha(sha, 0);
   if (idx != -1)
      laneTypes.setType(idx, LaneType::ACTIVE);
   else
      idx = add(LaneType::BRANCH, sha, activeLane);

   activeLane = idx;
}

void Lanes::afterMerge()
{
   for (int i = 0; i < laneTypes.count(); i++)
   {
      auto &t = laneTypes[i];

      if (t.isHead() || t.isJoin() || t.equals(LaneType::CROSS))
         t.setType(LaneType::NOT_ACTIVE);
      else if (t.equals(LaneType::CROSS_EMPTY))
         t.setType(LaneType::EMPTY);
      else if (laneTypes.isNode(t))
         t.setType(LaneType::ACTIVE);
   }
}

void Lanes::afterFork()
{
   for (int i = 0; i < laneTypes.count(); i++)
   {
      auto &t = laneTypes[i];

      if (t.equals(LaneType::CROSS))
         t.setType(LaneType::NOT_ACTIVE);
      else if (t.isTail() || t.equals(LaneType::CROSS_EMPTY))
         t.setType(LaneType::EMPTY);

      if (laneTypes.isNode(t))
         t.setType(LaneType::ACTIVE);
   }

   while (laneTypes.last().equals(LaneType::EMPTY))
   {
      laneTypes.removeLast();
      shaTracker.removeLast();
   }
}

bool Lanes::isBranch()
{
   if (laneTypes.count() > activeLane)
      return laneTypes.at(activeLane).equals(LaneType::BRANCH);
   return false;
}

void Lanes::afterBranch()
{
   laneTypes.setType(activeLane, LaneType::ACTIVE);
}

void Lanes::nextParent(const QString &sha)
{
   shaTracker.setNextSha(activeLane, sha);
}

int Lanes::add(LaneType type, const QString &next, int pos)
{
   if (pos < laneTypes.count())
   {
      pos = laneTypes.findType(LaneType::EMPTY, pos);
      if (pos != -1)
      {
         laneTypes.setType(pos, type);
         shaTracker.setNextSha(pos, next);
         return pos;
      }
   }

   laneTypes.append(type);
   shaTracker.append(next);
   return laneTypes.count() - 1;
}
