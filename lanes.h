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
   UNAPPLIED,
   APPLIED,
   BOUNDARY,
   BOUNDARY_C, // corresponds to MERGE_FORK
   BOUNDARY_R, // corresponds to MERGE_FORK_R
   BOUNDARY_L, // corresponds to MERGE_FORK_L

   LANE_TYPES_NUM
};

// graph helpers
inline bool isHead(const LaneType x)
{
   return (x == LaneType::HEAD || x == LaneType::HEAD_R || x == LaneType::HEAD_L);
}
inline bool isTail(const LaneType x)
{
   return (x == LaneType::TAIL || x == LaneType::TAIL_R || x == LaneType::TAIL_L);
}
inline bool isJoin(const LaneType x)
{
   return (x == LaneType::JOIN || x == LaneType::JOIN_R || x == LaneType::JOIN_L);
}
inline bool isFreeLane(const LaneType x)
{
   return (x == LaneType::NOT_ACTIVE || x == LaneType::CROSS || isJoin(x));
}
inline bool isBoundary(const LaneType x)
{
   return (x == LaneType::BOUNDARY || x == LaneType::BOUNDARY_C || x == LaneType::BOUNDARY_R
           || x == LaneType::BOUNDARY_L);
}
inline bool isMerge(const LaneType x)
{
   return (x == LaneType::MERGE_FORK || x == LaneType::MERGE_FORK_R || x == LaneType::MERGE_FORK_L || isBoundary(x));
}
inline bool isActive(const LaneType x)
{
   return (x == LaneType::ACTIVE || x == LaneType::INITIAL || x == LaneType::BRANCH || isMerge(x));
}

class Lanes
{
public:
   Lanes() {} // init() will setup us later, when data is available
   bool isEmpty() { return typeVec.empty(); }
   void init(const QString &expectedSha);
   void clear();
   bool isFork(const QString &sha, bool &isDiscontinuity);
   void setBoundary(bool isBoundary);
   void setFork(const QString &sha);
   void setMerge(const QStringList &parents);
   void setInitial();
   void setApplied();
   void changeActiveLane(const QString &sha);
   void afterMerge();
   void afterFork();
   bool isBranch();
   void afterBranch();
   void afterApplied();
   void nextParent(const QString &sha);
   void getLanes(QVector<int> &ln) { ln = typeVec; } // O(1) vector is implicitly shared

private:
   int findNextSha(const QString &next, int pos);
   int findType(const LaneType type, int pos);
   int add(const LaneType type, const QString &next, int pos);

   int activeLane;
   QVector<int> typeVec; // Describes which glyphs should be drawn.
   QVector<QString> nextShaVec; // The sha1 hashes of the next commit to appear in each lane (column).
   bool boundary;
   LaneType NODE, NODE_L, NODE_R;
};

#endif
