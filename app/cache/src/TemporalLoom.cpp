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
   qDebug() << QString("%1 workflow").arg(sha);
   qDebug() << QString("Parents:\n{ %1 }").arg(parents.join(", "));

   const auto isMerge = parents.count() > 1;
   const auto isFirstOfItsName = parents.count() == 0;
   const auto fork = isFork(sha);

   if (isMerge)
      setMerge(parents);
   else if (isFirstOfItsName)
      mTimeline.setType(activeLane, StateType::Initial);

   const auto lanes = mTimeline;

   const auto nextSha = isFirstOfItsName ? QString() : parents.first();

   mStateTracker.setNextSha(activeLane, nextSha);

   if (isMerge)
      afterMerge();
   if (fork)
      afterFork();
   if (isBranch())
      afterBranch();

   QStringList shas;
   for (auto i = 0; i < mStateTracker.count(); ++i)
      shas.append(mStateTracker.at(i));

   qDebug() << shas.join(",");

   QStringList states;
   for (auto i = 0; i < lanes.count(); i++)
      states.append(kStateTypeMap.value(lanes.at(i).getType()));

   qDebug() << QString("%1 - %2").arg(sha, states.join(" | "));

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

   const auto &startT = mTimeline.at(rangeStart);

   if (startT == StateType::MergeFork)
      mTimeline.setType(rangeStart, StateType::MergeForkLeft);

   if (startT == StateType::Tail)
      mTimeline.setType(rangeStart, StateType::TailLeft);

   const auto &endT = mTimeline.at(rangeEnd);

   if (endT == StateType::MergeFork)
      mTimeline.setType(rangeEnd, StateType::MergeForkRight);

   if (endT == StateType::Tail)
      mTimeline.setType(rangeEnd, StateType::TailRight);

   for (int i = rangeStart + 1; i < rangeEnd; ++i)
   {
      switch (mTimeline.at(i).getType())
      {
         case StateType::Inactive:
            mTimeline.setType(i, StateType::Cross);
            break;
         case StateType::Empty:
            mTimeline.setType(i, StateType::CrossEmpty);
            break;
         default:
            break;
      }
   }
}

void TemporalLoom::setMerge(const QStringList &parents)
{
   const auto &t = mTimeline.at(activeLane);
   auto wasFork = t == StateType::MergeFork;
   auto wasFork_L = t == StateType::MergeForkLeft;
   auto wasFork_R = t == StateType::MergeForkRight;
   auto startJoinWasACross = false;
   auto endJoinWasACross = false;

   mTimeline.setType(activeLane, StateType::MergeFork);

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
            endJoinWasACross = mTimeline.at(idx) == StateType::Cross;
         }

         if (idx < rangeStart)
         {
            rangeStart = idx;
            startJoinWasACross = mTimeline.at(idx) == StateType::Cross;
         }

         mTimeline.setType(idx, StateType::Join);
      }
      else
         rangeEnd = add(StateType::Head, *it, rangeEnd + 1);
   }

   const auto &startT = mTimeline.at(rangeStart);

   if (startT == StateType::MergeFork && !wasFork && !wasFork_R)
      mTimeline.setType(rangeStart, StateType::MergeForkLeft);

   if (startT == StateType::Join && !startJoinWasACross)
      mTimeline.setType(rangeStart, StateType::JoinLeft);

   if (startT == StateType::Head)
      mTimeline.setType(rangeStart, StateType::HeadLeft);

   const auto &endT = mTimeline.at(rangeEnd);

   if (endT == StateType::MergeFork && !wasFork && !wasFork_L)
      mTimeline.setType(rangeEnd, StateType::MergeForkRight);

   if (endT == StateType::Join && !endJoinWasACross)
      mTimeline.setType(rangeEnd, StateType::JoinRight);

   if (endT == StateType::Head)
      mTimeline.setType(rangeEnd, StateType::HeadRight);

   for (int i = rangeStart + 1; i < rangeEnd; i++)
   {
      const auto &t = mTimeline.at(i);

      if (t == StateType::Inactive)
         mTimeline.setType(i, StateType::Cross);
      else if (t == StateType::Empty)
         mTimeline.setType(i, StateType::CrossEmpty);
      else if (t == StateType::TailRight || t == StateType::TailLeft)
         mTimeline.setType(i, StateType::Tail);
   }
}

void TemporalLoom::setInitial()
{
   if (!mTimeline.at(activeLane).isMerge())
      mTimeline.setType(activeLane, StateType::Initial);
}

void TemporalLoom::changeActiveLane(const QString &sha)
{
   if (mTimeline.at(activeLane) == StateType::Initial)
      mTimeline.setType(activeLane, StateType::Empty);
   else
      mTimeline.setType(activeLane, StateType::Inactive);

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
      const auto &t = mTimeline.at(i);

      if (t.isHead() || t.isJoin() || t == StateType::Cross)
         mTimeline.setType(i, StateType::Inactive);
      else if (t == StateType::CrossEmpty)
         mTimeline.setType(i, StateType::Empty);
      else if (t.isMerge())
         mTimeline.setType(i, StateType::Active);
   }
}

void TemporalLoom::afterFork()
{
   for (int i = 0; i < mTimeline.count(); i++)
   {
      const auto &t = mTimeline.at(i);

      if (t == StateType::Cross)
         mTimeline.setType(i, StateType::Inactive);
      else if (t.isTail() || t == StateType::CrossEmpty)
         mTimeline.setType(i, StateType::Empty);

      if (t.isMerge())
         mTimeline.setType(i, StateType::Active);
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
