#include "ShaTracker.h"

void ShaTracker::clear()
{
   nextShaVec.clear();
   nextShaVec.squeeze();
}

int ShaTracker::findNextSha(const QString &next, int pos) const
{
   for (int i = pos; i < nextShaVec.count(); i++)
      if (nextShaVec[i] == next)
         return i;
   return -1;
}

void ShaTracker::setNextSha(int lane, const QString &sha)
{
   nextShaVec[lane] = sha;
}

void ShaTracker::append(const QString &sha)
{
   nextShaVec.append(sha);
}

void ShaTracker::removeLast()
{
   nextShaVec.pop_back();
}
