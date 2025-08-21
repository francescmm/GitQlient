#include <TemporalLoom.h>

#include <GitExecResult.h>

#include <QStringList>

namespace Graph
{
TemporalLoom::TemporalLoom()
{
   add(StateType::Branch, ZERO_SHA, 0);
}

Timeline TemporalLoom::createTimeline(const QString &sha, const QStringList &parents)
{
   const auto isMerge = parents.count() > 1;
   const auto isFirstOfItsName = parents.count() == 0;
   const auto fork = isFork(sha);

   if (isMerge)
      setMerge(parents);
   else if (isFirstOfItsName)
   {
      auto &u = mTimeline[activeLane];
      u.setType(StateType::Initial);
   }

   const auto lanes = mTimeline;

   const auto nextSha = isFirstOfItsName ? QString() : parents.first();

   mStateTracker.setNextSha(activeLane, nextSha);

   if (isMerge)
      afterMerge();
   if (fork)
      afterFork();
   if (isBranch())
      afterBranch();

   return lanes;
}

bool TemporalLoom::isFork(const QString &sha)
{
   int pos = mStateTracker.findNextSha(sha, 0);
   const auto isDiscontinuity = activeLane != pos;
   auto isFork = pos == -1 ? false : mStateTracker.findNextSha(sha, pos + 1) != -1;

   if (isDiscontinuity)
      changeActiveLane(sha);

   if (isFork)
      setFork(sha);

   return isFork;
}

void TemporalLoom::setFork(const QString &sha)
{
   auto rangeEnd = 0;
   auto idx = 0;
   auto rangeStart = rangeEnd = idx = mStateTracker.findNextSha(sha, 0);

   while (idx != -1)
   {
      rangeEnd = idx;
      mTimeline.setType(idx, StateType::Tail);
      idx = mStateTracker.findNextSha(sha, idx + 1);
   }

   mTimeline.setType(activeLane, StateType::MergeFork);

   auto &startT = mTimeline[rangeStart];
   auto &endT = mTimeline[rangeEnd];

   if (startT == StateType::MergeFork)
      startT.setType(StateType::MergeForkLeft);

   if (startT == StateType::Tail)
      startT.setType(StateType::TailLeft);

   if (endT == StateType::MergeFork)
      endT.setType(StateType::MergeForkRight);

   if (endT == StateType::Tail)
      endT.setType(StateType::TailRight);

   for (int i = rangeStart + 1; i < rangeEnd; ++i)
   {
      switch (auto &t = mTimeline[i]; t.getType())
      {
         case StateType::Inactive:
            t.setType(StateType::Cross);
            break;
         case StateType::Empty:
            t.setType(StateType::CrossEmpty);
            break;
         default:
            break;
      }
   }
}

void TemporalLoom::setMerge(const QStringList &parents)
{
   auto &t = mTimeline[activeLane];
   auto wasFork = t == StateType::MergeFork;
   auto wasFork_L = t == StateType::MergeForkLeft;
   auto wasFork_R = t == StateType::MergeForkRight;
   auto startJoinWasACross = false;
   auto endJoinWasACross = false;

   t.setType(StateType::MergeFork);

   auto rangeStart = activeLane;
   auto rangeEnd = activeLane;
   QStringList::const_iterator it(parents.constBegin());

   for (++it; it != parents.constEnd(); ++it)
   {
      int idx = mStateTracker.findNextSha(*it, 0);

      if (idx != -1)
      {
         if (idx > rangeEnd)
         {
            rangeEnd = idx;
            endJoinWasACross = mTimeline[idx] == StateType::Cross;
         }

         if (idx < rangeStart)
         {
            rangeStart = idx;
            startJoinWasACross = mTimeline[idx] == StateType::Cross;
         }

         mTimeline.setType(idx, StateType::Join);
      }
      else
         rangeEnd = add(StateType::Head, *it, rangeEnd + 1);
   }

   auto &startT = mTimeline[rangeStart];
   auto &endT = mTimeline[rangeEnd];

   if (startT == StateType::MergeFork && !wasFork && !wasFork_R)
      startT.setType(StateType::MergeForkLeft);

   if (endT == StateType::MergeFork && !wasFork && !wasFork_L)
      endT.setType(StateType::MergeForkRight);

   if (startT == StateType::Join && !startJoinWasACross)
      startT.setType(StateType::JoinLeft);

   if (endT == StateType::Join && !endJoinWasACross)
      endT.setType(StateType::JoinRight);

   if (startT == StateType::Head)
      startT.setType(StateType::HeadLeft);

   if (endT == StateType::Head)
      endT.setType(StateType::HeadRight);

   for (int i = rangeStart + 1; i < rangeEnd; i++)
   {
      auto &t = mTimeline[i];

      if (t == StateType::Inactive)
         t.setType(StateType::Cross);
      else if (t == StateType::Empty)
         t.setType(StateType::CrossEmpty);
      else if (t == StateType::TailRight || t == StateType::TailLeft)
         t.setType(StateType::Tail);
   }
}

void TemporalLoom::setInitial()
{
   auto &t = mTimeline[activeLane];
   if (!t.isMerge())
      t.setType(StateType::Initial);
}

void TemporalLoom::changeActiveLane(const QString &sha)
{
   auto &t = mTimeline[activeLane];

   if (t == StateType::Initial)
      t.setType(StateType::Empty);
   else
      t.setType(StateType::Inactive);

   int idx = mStateTracker.findNextSha(sha, 0);
   if (idx != -1)
      mTimeline.setType(idx, StateType::Active);
   else
      idx = add(StateType::Branch, sha, activeLane);

   activeLane = idx;
}

void TemporalLoom::afterMerge()
{
   for (int i = 0; i < mTimeline.count(); i++)
   {
      auto &t = mTimeline[i];

      if (t.isHead() || t.isJoin() || t == StateType::Cross)
         t.setType(StateType::Inactive);
      else if (t == StateType::CrossEmpty)
         t.setType(StateType::Empty);
      else if (t.isMerge())
         t.setType(StateType::Active);
   }
}

void TemporalLoom::afterFork()
{
   for (int i = 0; i < mTimeline.count(); i++)
   {
      auto &t = mTimeline[i];

      if (t == StateType::Cross)
         t.setType(StateType::Inactive);
      else if (t.isTail() || t == StateType::CrossEmpty)
         t.setType(StateType::Empty);

      if (t.isMerge())
         t.setType(StateType::Active);
   }

   while (mTimeline.last() == StateType::Empty)
   {
      mTimeline.removeLast();
      mStateTracker.removeLast();
   }
}

bool TemporalLoom::isBranch()
{
   return mTimeline.count() > activeLane ? mTimeline.at(activeLane) == StateType::Branch : false;
}

void TemporalLoom::afterBranch()
{
   mTimeline.setType(activeLane, StateType::Active);
}

int TemporalLoom::add(StateType type, const QString &next, int pos)
{
   if (pos < mTimeline.count())
   {
      pos = mTimeline.findType(StateType::Empty, pos);
      if (pos != -1)
      {
         mTimeline.setType(pos, type);
         mStateTracker.setNextSha(pos, next);
         return pos;
      }
   }

   mTimeline.append(type);
   mStateTracker.append(next);
   return mTimeline.count() - 1;
}
}
