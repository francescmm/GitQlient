#include "Commit.h"

#include <GitExecResult.h>

#include <QStringList>

#include <regex>

Commit::Commit(QByteArray commitData, const QString &gpg, bool goodSignature)
   : gpgKey(gpg)
   , mGoodSignature(goodSignature)
{
   parseDiff(commitData, 0);
}

Commit::Commit(QByteArray data)
{
   parseDiff(data, 1);
}

void Commit::parseDiff(QByteArray &data, qsizetype startingField)
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
         mParentsSha = shas.takeFirst().split(' ', Qt::SkipEmptyParts);

      committer = fields.at(startingField++);
      author = fields.at(startingField++);
      dateSinceEpoch = std::chrono::seconds(fields.at(startingField++).toInt());
      shortLog = fields.at(startingField++);

      for (auto i = startingField; i < fields.count(); ++i)
         longLog += fields.at(i) + '\n';

      longLog = longLog.trimmed();
   }
}

Commit::Commit(const QString &sha, const QStringList &parents, std::chrono::seconds commitDate,
                       const QString &log)
   : sha(sha)
   , dateSinceEpoch(commitDate)
   , shortLog(log)
   , mParentsSha(parents)
{
}

bool Commit::operator==(const Commit &commit) const
{
   return sha.startsWith(commit.sha) && mParentsSha == commit.mParentsSha && committer == commit.committer
       && author == commit.author && dateSinceEpoch == commit.dateSinceEpoch && shortLog == commit.shortLog
       && longLog == commit.longLog;
}

bool Commit::operator!=(const Commit &commit) const
{
   return !(*this == commit);
}

bool Commit::contains(const QString &value) const
{
   return sha.startsWith(value, Qt::CaseInsensitive) || shortLog.contains(value, Qt::CaseInsensitive)
       || committer.contains(value, Qt::CaseInsensitive) || author.contains(value, Qt::CaseInsensitive);
}

int Commit::parentsCount() const
{
   auto count = mParentsSha.count();

   if (count > 0 && mParentsSha.contains(ZERO_SHA))
      --count;

   return count;
}

QString Commit::firstParent() const
{
   return !mParentsSha.isEmpty() ? mParentsSha.at(0) : QString();
}

QStringList Commit::parents() const
{
   return mParentsSha;
}

void Commit::setParents(const QStringList &parents)
{
   mParentsSha = parents;
}

bool Commit::isInWorkingBranch() const
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

bool Commit::isValid() const
{
   const static std::regex hexMatcher("[0-9a-fA-F]{40}");
   const auto isMatch = std::regex_match(sha.toStdString(), hexMatcher);

   return !sha.isEmpty() && isMatch;
}

void Commit::removeChild(Commit *commit)
{
   if (mChilds.contains(commit))
      mChilds.removeAll(commit);
}

QString Commit::getFirstChildSha() const
{
   return !mChilds.isEmpty() ? mChilds.constFirst()->sha : QString {};
}
