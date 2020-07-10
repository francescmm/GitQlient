#include "CommitInfo.h"

#include <QStringList>

const QString CommitInfo::ZERO_SHA = QString("0000000000000000000000000000000000000000");

CommitInfo::CommitInfo(const QString &sha, const QStringList &parents, const QString &author, long long secsSinceEpoch,
                       const QString &log, const QString &longLog)
{
   mSha = sha;
   mParentsSha = parents;
   mCommitter = author;
   mAuthor = author;
   mCommitDate = QDateTime::fromSecsSinceEpoch(secsSinceEpoch);
   mShortLog = log;
   mLongLog = longLog;
}

CommitInfo::CommitInfo(const QByteArray &b)
{
   const auto fields = QString::fromUtf8(b).split('\n');

   if (fields.count() > 6)
   {
      auto combinedShas = fields.at(1);
      auto sha = combinedShas.split('X').first();
      mBoundaryInfo = sha.at(0);
      sha.remove(0, 1);
      mSha = sha;
      combinedShas = combinedShas.remove(0, mSha.size() + 1 + 1);
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
      mParentsSha = combinedShas.trimmed().split(' ', Qt::SkipEmptyParts);
#else
      mParentsSha = combinedShas.trimmed().split(' ', QString::SkipEmptyParts);
#endif
      mCommitter = fields.at(2);
      mAuthor = fields.at(3);
      mCommitDate = QDateTime::fromSecsSinceEpoch(fields.at(4).toInt());
      mShortLog = fields.at(5);

      for (auto i = 6; i < fields.count(); ++i)
         mLongLog += fields.at(i) + '\n';

      mLongLog = mLongLog.trimmed();
   }
}

bool CommitInfo::operator==(const CommitInfo &commit) const
{
   return (mSha == commit.mSha || mSha.startsWith(commit.sha()) || commit.sha().startsWith(mSha))
       && mParentsSha == commit.mParentsSha && mCommitter == commit.mCommitter && mAuthor == commit.mAuthor
       && mCommitDate == commit.mCommitDate && mShortLog == commit.mShortLog && mLongLog == commit.mLongLog
       && mLanes == commit.mLanes;
}

bool CommitInfo::operator!=(const CommitInfo &commit) const
{
   return !(*this == commit);
}

QString CommitInfo::getFieldStr(CommitInfo::Field field) const
{
   switch (field)
   {
      case CommitInfo::Field::SHA:
         return sha();
      case CommitInfo::Field::PARENTS_SHA:
         return parents().join(",");
      case CommitInfo::Field::COMMITER:
         return committer();
      case CommitInfo::Field::AUTHOR:
         return author();
      case CommitInfo::Field::DATE:
         return authorDate();
      case CommitInfo::Field::SHORT_LOG:
         return shortLog();
      case CommitInfo::Field::LONG_LOG:
         return longLog();
      default:
         return QString();
   }
}

bool CommitInfo::isValid() const
{
   static QRegExp hexMatcher("^[0-9A-F]{40}$", Qt::CaseInsensitive);

   return !mSha.isEmpty() && hexMatcher.exactMatch(mSha);
}

int CommitInfo::getActiveLane() const
{
   auto i = 0;

   for (auto lane : mLanes)
   {
      if (lane.isActive())
         return i;
      else
         ++i;
   }

   return -1;
}

void CommitInfo::addReference(References::Type type, const QString &reference)
{
   mReferences.addReference(type, reference);
}
