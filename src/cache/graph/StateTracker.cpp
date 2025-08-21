#include "StateTracker.h"

void StateTracker::clear()
{
   nextShaVec.clear();
   nextShaVec.squeeze();
}

int StateTracker::findNextSha(const QString &next, int pos) const
{
   for (int i = pos; i < nextShaVec.count(); i++)
      if (nextShaVec[i] == next)
         return i;
   return -1;
}

void StateTracker::setNextSha(int lane, const QString &sha)
{
   nextShaVec[lane] = sha;
}

void StateTracker::append(const QString &sha)
{
   nextShaVec.append(sha);
}

void StateTracker::removeLast()
{
   nextShaVec.pop_back();
}
