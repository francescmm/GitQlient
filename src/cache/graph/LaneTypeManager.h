#pragma once

#include <Lane.h>
#include <LaneType.h>

#include <QVector>

class LaneTypeManager
{
public:
   LaneTypeManager() = default;
   void clear();
   int findType(LaneType type, int pos) const;
   void setType(int lane, LaneType type);
   void append(LaneType type);
   void removeLast();
   int count() const { return typeVec.count(); }
   Lane &operator[](int index) { return typeVec[index]; }
   const Lane &at(int index) const { return typeVec.at(index); }
   const Lane &last() const { return typeVec.last(); }
   QVector<Lane> getLanes() const { return typeVec; }
   void setLanes(QVector<Lane> &ln) { ln = typeVec; }
   bool isNode(const Lane &lane) const;

private:
   QVector<Lane> typeVec;
   LaneType NODE = LaneType::MERGE_FORK;
   LaneType NODE_R = LaneType::MERGE_FORK_R;
   LaneType NODE_L = LaneType::MERGE_FORK_L;
};
