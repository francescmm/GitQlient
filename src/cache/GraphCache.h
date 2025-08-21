#pragma once

#include <lanes.h>

#include <QHash>
#include <QObject>
#include <QString>
#include <QVector>

#include <span>

class CommitInfo;

class GraphCache : public QObject
{
   Q_OBJECT

public:
   explicit GraphCache(QObject *parent = nullptr)
      : QObject(parent)
   {
   }

   void clear();
   void init(const QString &sha);
   void processLanes(std::span<CommitInfo> commits);
   void addCommit(const CommitInfo &commit);

   QVector<Lane> getLanes(const QString &sha) const;
   QVector<Lane> getLanes(int position) const;
   int getLanesCount(const QString &sha) const;
   Lane getLaneAt(const QString &sha, int index) const;
   int getActiveLane(const QString &sha) const;

private:
   struct LaneData
   {
      QVector<Lane> lanes;
      int activeLane = -1;
   };

   Lanes mLanes;
   QHash<QString, LaneData> mShaToLanes;
   QHash<int, QString> mPositionToSha;

   void calculateLanes(CommitInfo &commit);
   void updateCommitLanes(CommitInfo &commit);
   void resetLanes(const CommitInfo &commit, bool isFork);
};
