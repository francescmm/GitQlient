/*
        Description: history graph computation

        Author: Marco Costalba (C) 2005-2007

        Copyright: See COPYING file that comes with this distribution

*/
#include <QStringList>
#include "lanes.h"
#include <git.h>

#define IS_NODE(x) (x == static_cast<int>(NODE) || x == static_cast<int>(NODE_R) || x == static_cast<int>(NODE_L))

void Lanes::init(const QString &expectedSha)
{
   clear();
   activeLane = 0;
   setBoundary(false);
   add(LaneType::BRANCH, expectedSha, activeLane);
}

void Lanes::clear()
{

   typeVec.clear();
   nextShaVec.clear();
}

void Lanes::setBoundary(bool b)
{
   // changes the state so must be called as first one

   NODE = b ? LaneType::BOUNDARY_C : LaneType::MERGE_FORK;
   NODE_R = b ? LaneType::BOUNDARY_R : LaneType::MERGE_FORK_R;
   NODE_L = b ? LaneType::BOUNDARY_L : LaneType::MERGE_FORK_L;
   boundary = b;

   if (boundary)
      typeVec[activeLane] = static_cast<int>(LaneType::BOUNDARY);
}

bool Lanes::isFork(const QString &sha, bool &isDiscontinuity)
{

   int pos = findNextSha(sha, 0);
   isDiscontinuity = (activeLane != pos);
   if (pos == -1) // new branch case
      return false;

   return findNextSha(sha, pos + 1) != -1;
}

void Lanes::setFork(const QString &sha)
{

   int rangeStart, rangeEnd, idx;
   rangeStart = rangeEnd = idx = findNextSha(sha, 0);

   while (idx != -1)
   {
      rangeEnd = idx;
      typeVec[idx] = static_cast<int>(LaneType::TAIL);
      idx = findNextSha(sha, idx + 1);
   }
   typeVec[activeLane] = static_cast<int>(NODE);

   int &startT = typeVec[rangeStart];
   int &endT = typeVec[rangeEnd];

   if (startT == static_cast<int>(NODE))
      startT = static_cast<int>(NODE_L);

   if (endT == static_cast<int>(NODE))
      endT = static_cast<int>(NODE_R);

   if (startT == static_cast<int>(LaneType::TAIL))
      startT = static_cast<int>(LaneType::TAIL_L);

   if (endT == static_cast<int>(LaneType::TAIL))
      endT = static_cast<int>(LaneType::TAIL_R);

   for (int i = rangeStart + 1; i < rangeEnd; i++)
   {

      int &t = typeVec[i];

      if (t == static_cast<int>(LaneType::NOT_ACTIVE))
         t = static_cast<int>(LaneType::CROSS);

      else if (t == static_cast<int>(LaneType::EMPTY))
         t = static_cast<int>(LaneType::CROSS_EMPTY);
   }
}

void Lanes::setMerge(const QStringList &parents)
{
   // setFork() must be called before setMerge()

   if (boundary)
      return; // handle as a simple active line

   int &t = typeVec[activeLane];
   bool wasFork = (t == static_cast<int>(NODE));
   bool wasFork_L = (t == static_cast<int>(NODE_L));
   bool wasFork_R = (t == static_cast<int>(NODE_R));
   bool startJoinWasACross = false, endJoinWasACross = false;

   t = static_cast<int>(NODE);

   int rangeStart = activeLane, rangeEnd = activeLane;
   QStringList::const_iterator it(parents.constBegin());
   for (++it; it != parents.constEnd(); ++it)
   { // skip first parent

      int idx = findNextSha(*it, 0);
      if (idx != -1)
      {

         if (idx > rangeEnd)
         {

            rangeEnd = idx;
            endJoinWasACross = typeVec[idx] == static_cast<int>(LaneType::CROSS);
         }

         if (idx < rangeStart)
         {

            rangeStart = idx;
            startJoinWasACross = typeVec[idx] == static_cast<int>(LaneType::CROSS);
         }

         typeVec[idx] = static_cast<int>(LaneType::JOIN);
      }
      else
         rangeEnd = add(LaneType::HEAD, *it, rangeEnd + 1);
   }
   int &startT = typeVec[rangeStart];
   int &endT = typeVec[rangeEnd];

   if (startT == static_cast<int>(NODE) && !wasFork && !wasFork_R)
      startT = static_cast<int>(NODE_L);

   if (endT == static_cast<int>(NODE) && !wasFork && !wasFork_L)
      endT = static_cast<int>(NODE_R);

   if (startT == static_cast<int>(LaneType::JOIN) && !startJoinWasACross)
      startT = static_cast<int>(LaneType::JOIN_L);

   if (endT == static_cast<int>(LaneType::JOIN) && !endJoinWasACross)
      endT = static_cast<int>(LaneType::JOIN_R);

   if (startT == static_cast<int>(LaneType::HEAD))
      startT = static_cast<int>(LaneType::HEAD_L);

   if (endT == static_cast<int>(LaneType::HEAD))
      endT = static_cast<int>(LaneType::HEAD_R);

   for (int i = rangeStart + 1; i < rangeEnd; i++)
   {

      int &t = typeVec[i];

      if (t == static_cast<int>(LaneType::NOT_ACTIVE))
         t = static_cast<int>(LaneType::CROSS);

      else if (t == static_cast<int>(LaneType::EMPTY))
         t = static_cast<int>(LaneType::CROSS_EMPTY);

      else if (t == static_cast<int>(LaneType::TAIL_R) || t == static_cast<int>(LaneType::TAIL_L))
         t = static_cast<int>(LaneType::TAIL);
   }
}

void Lanes::setInitial()
{

   int &t = typeVec[activeLane];
   if (!IS_NODE(t) && t != static_cast<int>(LaneType::APPLIED))
      t = static_cast<int>(boundary ? LaneType::BOUNDARY : LaneType::INITIAL);
}

void Lanes::setApplied()
{

   // applied patches are not merges, nor forks
   typeVec[activeLane] = static_cast<int>(LaneType::APPLIED); // TODO test with boundaries
}

void Lanes::changeActiveLane(const QString &sha)
{

   int &t = typeVec[activeLane];
   if (t == static_cast<int>(LaneType::INITIAL) || isBoundary(static_cast<LaneType>(t)))
      t = static_cast<int>(LaneType::EMPTY);
   else
      t = static_cast<int>(LaneType::NOT_ACTIVE);

   int idx = findNextSha(sha, 0); // find first sha
   if (idx != -1)
      typeVec[idx] = static_cast<int>(LaneType::ACTIVE); // called before setBoundary()
   else
      idx = add(LaneType::BRANCH, sha, activeLane); // new branch

   activeLane = idx;
}

void Lanes::afterMerge()
{

   if (boundary)
      return; // will be reset by changeActiveLane()

   for (int i = 0; i < typeVec.count(); i++)
   {

      int &t = typeVec[i];

      if (isHead(static_cast<LaneType>(t)) || isJoin(static_cast<LaneType>(t))
          || t == static_cast<int>(LaneType::CROSS))
         t = static_cast<int>(LaneType::NOT_ACTIVE);

      else if (t == static_cast<int>(LaneType::CROSS_EMPTY))
         t = static_cast<int>(LaneType::EMPTY);

      else if (IS_NODE(t))
         t = static_cast<int>(LaneType::ACTIVE);
   }
}

void Lanes::afterFork()
{

   for (int i = 0; i < typeVec.count(); i++)
   {

      int &t = typeVec[i];

      if (t == static_cast<int>(LaneType::CROSS))
         t = static_cast<int>(LaneType::NOT_ACTIVE);

      else if (isTail(static_cast<LaneType>(t)) || t == static_cast<int>(LaneType::CROSS_EMPTY))
         t = static_cast<int>(LaneType::EMPTY);

      if (!boundary && IS_NODE(t))
         t = static_cast<int>(LaneType::ACTIVE); // boundary will be reset by changeActiveLane()
   }
   while (typeVec.last() == static_cast<int>(LaneType::EMPTY))
   {
      typeVec.pop_back();
      nextShaVec.pop_back();
   }
}

bool Lanes::isBranch()
{

   return (typeVec[activeLane] == static_cast<int>(LaneType::BRANCH));
}

void Lanes::afterBranch()
{

   typeVec[activeLane] = static_cast<int>(LaneType::ACTIVE); // TODO test with boundaries
}

void Lanes::afterApplied()
{

   typeVec[activeLane] = static_cast<int>(LaneType::ACTIVE); // TODO test with boundaries
}

void Lanes::nextParent(const QString &sha)
{

   nextShaVec[activeLane] = (boundary ? "" : sha);
}

int Lanes::findNextSha(const QString &next, int pos)
{

   for (int i = pos; i < nextShaVec.count(); i++)
      if (nextShaVec[i] == next)
         return i;
   return -1;
}

int Lanes::findType(const LaneType type, int pos)
{

   for (int i = pos; i < typeVec.count(); i++)
      if (typeVec[i] == static_cast<int>(type))
         return i;
   return -1;
}

int Lanes::add(const LaneType type, const QString &next, int pos)
{

   // first check empty lanes starting from pos
   if (pos < (int)typeVec.count())
   {
      pos = findType(LaneType::EMPTY, pos);
      if (pos != -1)
      {
         typeVec[pos] = static_cast<int>(type);
         nextShaVec[pos] = next;
         return pos;
      }
   }
   // if all lanes are occupied add a new lane
   typeVec.append(static_cast<int>(type));
   nextShaVec.append(next);
   return typeVec.count() - 1;
}
