/*
        Author: Marco Costalba (C) 2005-2007

        Copyright: See COPYING file that comes with this distribution

*/
#ifndef LANES_H
#define LANES_H

#include <QString>
#include <QVector>

class QStringList;

//
//  At any given time, the Lanes class represents a single revision (row) of the history graph.
//  The Lanes class contains a vector of the sha1 hashes of the next commit to appear in each lane (column).
//  The Lanes class also contains a vector used to decide which glyph to draw on the history graph.
//
//  For each revision (row) (from recent (top) to ancient past (bottom)), the Lanes class is updated, and the
//  current revision (row) of glyphs is saved elsewhere (via getLanes()).
//
//  The ListView class is responsible for rendering the glyphs.
//

enum class LaneType
{
   EMPTY,
   ACTIVE,
   NOT_ACTIVE,
   MERGE_FORK,
   MERGE_FORK_R,
   MERGE_FORK_L,
   JOIN,
   JOIN_R,
   JOIN_L,
   HEAD,
   HEAD_R,
   HEAD_L,
   TAIL,
   TAIL_R,
   TAIL_L,
   CROSS,
   CROSS_EMPTY,
   INITIAL,
   BRANCH,
   BOUNDARY,
   BOUNDARY_C, // corresponds to MERGE_FORK
   BOUNDARY_R, // corresponds to MERGE_FORK_R
   BOUNDARY_L, // corresponds to MERGE_FORK_L

   LANE_TYPES_NUM
};

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

   void setBoundary() { mType = LaneType::BOUNDARY; };
   void setType(LaneType type) { mType = type; }

private:
   LaneType mType;
};

class Lanes
{
public:
   Lanes() { } // init() will setup us later, when data is available
   bool isEmpty() { return typeVec.empty(); }
   void init(const QString &expectedSha);
   void clear();
   bool isFork(const QString &sha, bool &isDiscontinuity);
   void setBoundary(bool isBoundary);
   void setFork(const QString &sha);
   void setMerge(const QStringList &parents);
   void setInitial();
   void changeActiveLane(const QString &sha);
   void afterMerge();
   void afterFork();
   bool isBranch();
   void afterBranch();
   void nextParent(const QString &sha);
   void setLanes(QVector<Lane> &ln) { ln = typeVec; } // O(1) vector is implicitly shared
   QVector<Lane> getLanes() const { return typeVec; }

private:
   int findNextSha(const QString &next, int pos);
   int findType(LaneType type, int pos);
   int add(LaneType type, const QString &next, int pos);
   bool isNode(Lane lane) const;

   int activeLane;
   QVector<Lane> typeVec; // Describes which glyphs should be drawn.
   QVector<QString> nextShaVec; // The sha1 hashes of the next commit to appear in each lane (column).
   bool boundary;
   LaneType NODE, NODE_L, NODE_R;
};

#endif
