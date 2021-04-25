#include "CommitInfo.h"

#include <QStringList>

const QString CommitInfo::ZERO_SHA = QString("0000000000000000000000000000000000000000");
const QString CommitInfo::INIT_SHA = QString("4b825dc642cb6eb9a060e54bf8d69288fbee4904");

CommitInfo::CommitInfo(const QString sha, const QStringList &parents, const QString &commiter,
                       const QDateTime &commitDate, const QString &author, const QString &log, const QString &longLog,
                       bool isSigned, const QString &gpgKey)
   : CommitInfo(sha, parents, commitDate, log)
{
   mCommitter = commiter;
   mAuthor = author;
   mLongLog = longLog;
   mSigned = isSigned;
   mGpgKey = gpgKey;
}

CommitInfo::~CommitInfo()
{
   mLanes.clear();
   mLanes.squeeze();
   mChilds.clear();
   mChilds.squeeze();
}

CommitInfo::CommitInfo(const QString sha, const QStringList &parents, const QDateTime &commitDate, const QString &log)
   : mSha(sha)
   , mParentsSha(parents)
   , mShortLog(log)
   , mCommitDate(commitDate)
{
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

bool CommitInfo::contains(const QString &value)
{
   return mSha.startsWith(value, Qt::CaseInsensitive) || mShortLog.contains(value, Qt::CaseInsensitive)
       || mCommitter.contains(value, Qt::CaseInsensitive) || mAuthor.contains(value, Qt::CaseInsensitive);
}

int CommitInfo::parentsCount() const
{
   auto count = mParentsSha.count();

   if (count > 0 && mParentsSha.contains(CommitInfo::INIT_SHA))
      --count;

   return count;
}

QString CommitInfo::parent(int idx) const
{
   return mParentsSha.count() > idx ? mParentsSha.at(idx) : QString();
}

QStringList CommitInfo::parents() const
{
   return mParentsSha;
}

bool CommitInfo::isInWorkingBranch() const
{
   for (const auto &child : mChilds)
   {
      if (child->mSha == CommitInfo::ZERO_SHA)
      {
         return true;
         break;
      }
   }

   return false;
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

QString CommitInfo::getFirstChildSha() const
{
   if (!mChilds.isEmpty())
      mChilds.constFirst();
   return QString();
}
