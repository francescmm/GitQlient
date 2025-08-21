#pragma once

#include <QDateTime>
#include <QStringList>
#include <QVector>

#include <chrono>

#include <References.h>

class CommitInfo
{
public:
   enum class Field
   {
      SHA,
      PARENTS_SHA,
      COMMITTER,
      AUTHOR,
      DATE,
      SHORT_LOG,
      LONG_LOG
   };

   CommitInfo() = default;
   ~CommitInfo() = default;
   CommitInfo(QByteArray commitData);
   CommitInfo(QByteArray commitData, const QString &gpg, bool goodSignature);
   explicit CommitInfo(const QString &sha, const QStringList &parents, std::chrono::seconds commitDate,
                       const QString &log);
   bool operator==(const CommitInfo &commit) const;
   bool operator!=(const CommitInfo &commit) const;

   bool isValid() const;
   bool contains(const QString &value) const;

   int parentsCount() const;
   QString firstParent() const;
   QStringList parents() const;
   void setParents(const QStringList &parents);
   bool isInWorkingBranch() const;

   void appendChild(CommitInfo *commit) { mChilds.append(commit); }
   void removeChild(CommitInfo *commit);
   bool hasChilds() const { return !mChilds.empty(); }
   QString getFirstChildSha() const;
   int getChildsCount() const { return mChilds.count(); }

   bool isSigned() const { return !gpgKey.isEmpty(); }
   bool verifiedSignature() const { return mGoodSignature && !gpgKey.isEmpty(); }

   uint pos = 0;
   QString sha;
   QString committer;
   QString author;
   std::chrono::seconds dateSinceEpoch;
   QString shortLog;
   QString longLog;
   QString gpgKey;

private:
   bool mGoodSignature = false;
   QStringList mParentsSha;
   QVector<CommitInfo *> mChilds;

   friend class GitCache;

   void parseDiff(QByteArray &data, qsizetype startingField);
};
