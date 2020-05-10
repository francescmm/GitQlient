#pragma once

enum class LaneType;

class Lane
{
public:
   Lane(LaneType type);

   bool operator==(const Lane &lane) const { return mType == lane.mType; }

   bool isHead() const;
   bool isTail() const;
   bool isJoin() const;
   bool isFreeLane() const;
   bool isBoundary() const;
   bool isMerge() const;
   bool isActive() const;
   bool equals(LaneType type) const { return mType == type; }
   LaneType getType() const { return mType; }

   void setBoundary();
   void setType(LaneType type) { mType = type; }

private:
   LaneType mType;
};
