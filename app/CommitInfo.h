#pragma once

#include <QVector>
#include <QStringList>
#include <QDateTime>

enum class LaneType;

class CommitInfo
{
public:
   CommitInfo() = default;
   CommitInfo(const QString &sha, const QStringList &parents, const QString &author, long long secsSinceEpoch,
              const QString &log, const QString &longLog, int idx);
   CommitInfo(const QByteArray &b, int idx);
   bool isBoundary() const { return mBoundaryInfo == '-'; }
   int parentsCount() const { return mParentsSha.count(); }
   QString parent(int idx) const { return mParentsSha.count() > idx ? mParentsSha.at(idx) : QString(); }
   QStringList parents() const { return mParentsSha; }
   QString sha() const { return mSha; }
   QString committer() const { return mCommitter; }
   QString author() const { return mAuthor; }
   QString authorDate() const { return QString::number(mCommitDate.toSecsSinceEpoch()); }
   QString shortLog() const { return mShortLog; }
   QString longLog() const { return mLongLog; }
   bool isValid() const { return !mSha.isEmpty(); }

   QVector<LaneType> lanes;
   int orderIdx = -1;
   bool isDiffCache = false;
   bool isApplied = false;

private:
   QChar mBoundaryInfo;
   QString mSha;
   QStringList mParentsSha;
   QString mCommitter;
   QString mAuthor;
   QDateTime mCommitDate;
   QString mShortLog;
   QString mLongLog;
   QString mDiff;
};
