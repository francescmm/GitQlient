#include "CommitInfo.h"

#include <QStringList>

const QString CommitInfo::ZERO_SHA = QString("0000000000000000000000000000000000000000");

CommitInfo::CommitInfo(const QString sha, const QStringList &parents, const QChar &boundary, const QString &commiter,
                       const QDateTime &commitDate, const QString &author, const QString &log, const QString &longLog,
                       bool isSigned, const QString &gpgKey)
{
   mSha = sha;
   mParentsSha = parents;
   mBoundaryInfo = boundary;
   mCommitter = commiter;
   mCommitDate = commitDate;
   mAuthor = author;
   mShortLog = log;
   mLongLog = longLog;
   mSigned = isSigned;
   mGpgKey = gpgKey;
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
