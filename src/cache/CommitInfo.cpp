#include "CommitInfo.h"

#include <QStringList>

const QString CommitInfo::ZERO_SHA = QString("0000000000000000000000000000000000000000");
const QString CommitInfo::INIT_SHA = QString("4b825dc642cb6eb9a060e54bf8d69288fbee4904");

CommitInfo::CommitInfo(const QString sha, const QStringList &parents, const QString &commiter,
                       std::chrono::seconds commitDate, const QString &author, const QString &log,
                       const QString &longLog, bool isSigned, const QString &gpgKey)
   : CommitInfo(sha, parents, commitDate, log)
{
   committer = commiter;
   this->author = author;
   this->longLog = longLog;
   this->isSigned = isSigned;
   this->gpgKey = gpgKey;
}

CommitInfo::~CommitInfo()
{
   lanes.clear();
   lanes.squeeze();
   mChilds.clear();
   mChilds.squeeze();
}

CommitInfo::CommitInfo(const QString sha, const QStringList &parents, std::chrono::seconds commitDate,
                       const QString &log)
   : sha(sha)
   , dateSinceEpoch(commitDate)
   , shortLog(log)
   , mParentsSha(parents)
{
}

bool CommitInfo::operator==(const CommitInfo &commit) const
{
   return sha.startsWith(commit.sha) && mParentsSha == commit.mParentsSha && committer == commit.committer
       && author == commit.author && dateSinceEpoch == commit.dateSinceEpoch && shortLog == commit.shortLog
       && longLog == commit.longLog && lanes == commit.lanes;
}

bool CommitInfo::operator!=(const CommitInfo &commit) const
{
   return !(*this == commit);
}

bool CommitInfo::contains(const QString &value)
{
   return sha.startsWith(value, Qt::CaseInsensitive) || shortLog.contains(value, Qt::CaseInsensitive)
       || committer.contains(value, Qt::CaseInsensitive) || author.contains(value, Qt::CaseInsensitive);
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
      if (child->sha == CommitInfo::ZERO_SHA)
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

   return !sha.isEmpty() && hexMatcher.exactMatch(sha);
}

int CommitInfo::getActiveLane() const
{
   auto i = 0;

   for (auto lane : lanes)
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
