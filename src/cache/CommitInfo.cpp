#include "CommitInfo.h"

#include <GitExecResult.h>

#include <QStringList>

#include <regex>

CommitInfo::CommitInfo(QByteArray commitData, const QString &gpg, bool goodSignature)
   : gpgKey(gpg)
   , mGoodSignature(goodSignature)
{
   parseDiff(commitData, 0);
}

CommitInfo::CommitInfo(QByteArray data)
{
   parseDiff(data, 1);
}

void CommitInfo::parseDiff(QByteArray &data, qsizetype startingField)
{
   if (data.isEmpty())
      return;

   if (const auto fields = QString::fromUtf8(data).split('\n'); fields.count() > 0)
   {
      auto combinedShas = fields.at(startingField++);
      auto shas = combinedShas.split('X');
      auto first = shas.takeFirst();
      sha = first.remove(0, 1);

      if (!shas.isEmpty())
      {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
         mParentsSha = shas.takeFirst().split(' ', Qt::SkipEmptyParts);
#else
         mParentsSha = shas.takeFirst().split(' ', QString::SkipEmptyParts);
#endif
      }
      committer = fields.at(startingField++);
      author = fields.at(startingField++);
      dateSinceEpoch = std::chrono::seconds(fields.at(startingField++).toInt());
      shortLog = fields.at(startingField++);

      for (auto i = startingField; i < fields.count(); ++i)
         longLog += fields.at(i) + '\n';

      longLog = longLog.trimmed();
   }
}

CommitInfo::CommitInfo(const QString &sha, const QStringList &parents, std::chrono::seconds commitDate,
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
       && longLog == commit.longLog && mLanes == commit.mLanes;
}

bool CommitInfo::operator!=(const CommitInfo &commit) const
{
   return !(*this == commit);
}

bool CommitInfo::contains(const QString &value) const
{
   return sha.startsWith(value, Qt::CaseInsensitive) || shortLog.contains(value, Qt::CaseInsensitive)
       || committer.contains(value, Qt::CaseInsensitive) || author.contains(value, Qt::CaseInsensitive);
}

int CommitInfo::parentsCount() const
{
   auto count = mParentsSha.count();

   if (count > 0 && mParentsSha.contains(ZERO_SHA))
      --count;

   return count;
}

QString CommitInfo::firstParent() const
{
   return !mParentsSha.isEmpty() ? mParentsSha.at(0) : QString();
}

QStringList CommitInfo::parents() const
{
   return mParentsSha;
}

void CommitInfo::setParents(const QStringList &parents)
{
   mParentsSha = parents;
}

bool CommitInfo::isInWorkingBranch() const
{
   for (const auto &child : mChilds)
   {
      if (child->sha == ZERO_SHA)
      {
         return true;
         break;
      }
   }

   return false;
}

void CommitInfo::setLanes(QVector<Lane> lanes)
{
   this->mLanes.clear();
   this->mLanes.squeeze();
   this->mLanes = std::move(lanes);
}

bool CommitInfo::isValid() const
{
   const static std::regex hexMatcher("[0-9a-fA-F]{40}");
   const auto isMatch = std::regex_match(sha.toStdString(), hexMatcher);

   return !sha.isEmpty() && isMatch;
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

void CommitInfo::removeChild(CommitInfo *commit)
{
   if (mChilds.contains(commit))
      mChilds.removeAll(commit);
}

QString CommitInfo::getFirstChildSha() const
{
   return !mChilds.isEmpty() ? mChilds.constFirst()->sha : QString {};
}
