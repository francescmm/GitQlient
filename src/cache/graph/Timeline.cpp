#include "Timeline.h"

namespace Graph
{

int Timeline::findType(StateType type, int pos) const
{
   const auto typeVecCount = mStates.count();
   for (int i = pos; i < typeVecCount; i++)
      if (mStates[i] == type)
         return i;
   return -1;
}

void Timeline::setType(int lane, StateType type)
{
   mStates[lane].setType(type);
}

void Timeline::append(StateType type)
{
   mStates.append(type);
}

void Timeline::removeLast()
{
   mStates.pop_back();
}
}
