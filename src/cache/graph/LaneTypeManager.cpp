#include "LaneTypeManager.h"

void LaneTypeManager::clear()
{
   typeVec.clear();
   typeVec.squeeze();
}

int LaneTypeManager::findType(LaneType type, int pos) const
{
   const auto typeVecCount = typeVec.count();
   for (int i = pos; i < typeVecCount; i++)
      if (typeVec[i].equals(type))
         return i;
   return -1;
}

void LaneTypeManager::setType(int lane, LaneType type)
{
   typeVec[lane].setType(type);
}

void LaneTypeManager::append(LaneType type)
{
   typeVec.append(type);
}

void LaneTypeManager::removeLast()
{
   typeVec.pop_back();
}

bool LaneTypeManager::isNode(const Lane &lane) const
{
   return lane.equals(NODE) || lane.equals(NODE_R) || lane.equals(NODE_L);
}
