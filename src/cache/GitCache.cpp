#include "GitCache.h"

#include <QLogger.h>
#include <WipRevisionInfo.h>

using namespace QLogger;

GitCache::GitCache(QObject *parent)
   : QObject(parent)
   , mMutex(QMutex::Recursive)
{
}

GitCache::~GitCache()
{
   mCommits.clear();
   mCommitsMap.clear();
   mReferences.clear();
}

void GitCache::setup(const WipRevisionInfo &wipInfo, const QList<CommitInfo> &commits)
{
   QMutexLocker lock(&mMutex);

   const auto totalCommits = commits.count() + 1;

   QLog_Debug("Cache", QString("Configuring the cache for {%1} elements.").arg(totalCommits));

   mConfigured = false;

   mRevisionFilesMap.clear();
   mLanes.clear();

   if (mCommitsMap.isEmpty())
      mCommitsMap.reserve(totalCommits);

   if (mCommits.isEmpty() || totalCommits > mCommits.count())
      mCommits.resize(totalCommits);
   else if (totalCommits < mCommits.count())
   {
      const auto commitsToRemove = std::abs(totalCommits - mCommits.count());

      for (auto i = 0; i < commitsToRemove; ++i)
         mCommits.takeLast();
   }

   QLog_Debug("Cache", QString("Adding WIP revision."));

   insertWipRevision(wipInfo);

   auto count = 1;

   QLog_Debug("Cache", QString("Adding committed revisions."));

   for (auto commit : commits)
   {
      if (commit.isValid())
      {
         commit.setLanes(calculateLanes(commit));

         const auto sha = commit.sha();

         if (sha == mCommitsMap.value(CommitInfo::ZERO_SHA).parent(0))
            commit.addChildReference(&mCommitsMap[CommitInfo::ZERO_SHA]);

         mCommitsMap[sha] = std::move(commit);

         mCommits.replace(count, &mCommitsMap[sha]);

         if (mTmpChildsStorage.contains(sha))
         {
            for (auto &child : mTmpChildsStorage.values(sha))
               mCommitsMap[sha].addChildReference(child);

            mTmpChildsStorage.remove(sha);
         }

         const auto parents = mCommitsMap.value(sha).parents();

         for (const auto &parent : parents)
            mTmpChildsStorage.insert(parent, &mCommitsMap[sha]);

         ++count;
      }
   }
}

CommitInfo GitCache::commitInfo(int row)
{
   QMutexLocker lock(&mMutex);

   const auto commit = row >= 0 && row < mCommits.count() ? mCommits.at(row) : nullptr;

   return commit ? *commit : CommitInfo();
}

auto GitCache::searchCommit(const QString &text, const int startingPoint) const
{
   return std::find_if(mCommits.constBegin() + startingPoint, mCommits.constEnd(),
                       [text](CommitInfo *info) { return info->contains(text); });
}

auto GitCache::reverseSearchCommit(const QString &text, int startingPoint) const
{
   const auto startEndPos = startingPoint > 0 ? mCommits.count() - startingPoint + 1 : 0;

   return std::find_if(mCommits.crbegin() + startEndPos, mCommits.crend(),
                       [text](CommitInfo *info) { return info->contains(text); });
}

CommitInfo GitCache::searchCommitInfo(const QString &text, int startingPoint, bool reverse)
{
   QMutexLocker lock(&mMutex);
   CommitInfo commit;

   if (!reverse)
   {
      auto commitIter = searchCommit(text, startingPoint);

      if (commitIter == mCommits.constEnd())
         commitIter = searchCommit(text);

      if (commitIter != mCommits.constEnd())
         commit = **commitIter;
   }
   else
   {
      auto commitIter = reverseSearchCommit(text, startingPoint);

      if (commitIter == mCommits.crend())
         commitIter = reverseSearchCommit(text);

      if (commitIter != mCommits.crend())
         commit = **commitIter;
   }

   return commit;
}

bool GitCache::isCommitInCurrentGeneologyTree(const QString &sha) const
{
   return checkSha(sha, CommitInfo::ZERO_SHA);
}

CommitInfo GitCache::commitInfo(const QString &sha)
{
   QMutexLocker lock(&mMutex);

   if (!sha.isEmpty())
   {
      const auto c = mCommitsMap.value(sha, CommitInfo());

      if (!c.isValid())
      {
         const auto shas = mCommitsMap.keys();
         const auto it = std::find_if(shas.cbegin(), shas.cend(),
                                      [sha](const QString &shaToCompare) { return shaToCompare.startsWith(sha); });

         if (it != shas.cend())
            return mCommitsMap.value(*it);

         return CommitInfo();
      }

      return c;
   }

   return CommitInfo();
}

RevisionFiles GitCache::revisionFile(const QString &sha1, const QString &sha2) const
{
   const auto iter = mRevisionFilesMap.constFind(qMakePair(sha1, sha2));

   return iter != mRevisionFilesMap.cend() ? *iter : RevisionFiles();
}

void GitCache::clearReferences()
{
   QMutexLocker lock(&mMutex);
   mReferences.clear();
}

void GitCache::insertWipRevision(const WipRevisionInfo &wipInfo)
{
   auto newParentSha = wipInfo.parentSha;

   QLog_Debug("Cache", QString("Updating the WIP commit. The actual parent has SHA {%1}.").arg(newParentSha));

   const auto key = qMakePair(CommitInfo::ZERO_SHA, newParentSha);
   const auto fakeRevFile = fakeWorkDirRevFile(wipInfo.diffIndex, wipInfo.diffIndexCached);

   insertRevisionFile(CommitInfo::ZERO_SHA, newParentSha, fakeRevFile);

   const auto log = fakeRevFile.count() == mUntrackedfiles.count() ? tr("No local changes") : tr("Local changes");

   QStringList parents;

   if (!newParentSha.isEmpty())
      parents.append(newParentSha);

   CommitInfo c(CommitInfo::ZERO_SHA, parents, QChar(), QStringLiteral("-"), QDateTime::currentDateTime(),
                QStringLiteral("-"), log);

   if (mLanes.isEmpty())
      mLanes.init(c.sha());

   c.setLanes(calculateLanes(c));

   if (mCommits[0])
      c.setLanes(mCommits[0]->getLanes());

   const auto sha = c.sha();

   mCommitsMap.insert(sha, std::move(c));
   mCommits[0] = &mCommitsMap[sha];
}

bool GitCache::insertRevisionFile(const QString &sha1, const QString &sha2, const RevisionFiles &file)
{
   const auto key = qMakePair(sha1, sha2);
   const auto emptyShas = !sha1.isEmpty() && !sha2.isEmpty();
   const auto isWip = sha1 == CommitInfo::ZERO_SHA;

   if ((emptyShas || isWip) && mRevisionFilesMap.value(key) != file)
   {
      QLog_Debug("Cache", QString("Adding the revisions files between {%1} and {%2}.").arg(sha1, sha2));

      mRevisionFilesMap.insert(key, file);

      return true;
   }

   return false;
}

void GitCache::insertReference(const QString &sha, References::Type type, const QString &reference)
{
   QMutexLocker lock(&mMutex);

   QLog_Debug("Cache", QString("Adding a new reference with SHA {%1}.").arg(sha));

   mReferences[sha].addReference(type, reference);
}

bool GitCache::hasReferences(const QString &sha) const
{
   return mReferences.contains(sha) && !mReferences.value(sha).isEmpty();
}

QStringList GitCache::getReferences(const QString &sha, References::Type type) const
{
   return mReferences.value(sha).getReferences(type);
}

void GitCache::reloadCurrentBranchInfo(const QString &currentBranch, const QString &currentSha)
{
   const auto lastItem = mReferences.end();
   for (auto ref = mReferences.begin(); ref != lastItem; ++ref)
   {
      if (ref.value().getReferences(References::Type::LocalBranch).contains(currentBranch))
      {
         ref.value().removeReference(References::Type::LocalBranch, currentBranch);

         const auto key = ref.key();

         if (mReferences.value(key).isEmpty())
            mReferences.remove(key);

         break;
      }
   }

   mReferences[currentSha].addReference(References::Type::LocalBranch, currentBranch);
}

bool GitCache::updateWipCommit(const WipRevisionInfo &wipInfo)
{
   if (mConfigured)
   {
      insertWipRevision(wipInfo);
      return true;
   }

   return false;
}

QVector<Lane> GitCache::calculateLanes(const CommitInfo &c)
{
   const auto sha = c.sha();

   QLog_Trace("Cache", QString("Updating the lanes for SHA {%1}.").arg(sha));

   bool isDiscontinuity;
   bool isFork = mLanes.isFork(sha, isDiscontinuity);
   bool isMerge = c.parentsCount() > 1;

   if (isDiscontinuity)
      mLanes.changeActiveLane(sha); // uses previous isBoundary state

   if (isFork)
      mLanes.setFork(sha);
   if (isMerge)
      mLanes.setMerge(c.parents());
   if (c.parentsCount() == 0)
      mLanes.setInitial();

   const auto lanes = mLanes.getLanes();

   resetLanes(c, isFork);

   return lanes;
}

bool GitCache::pendingLocalChanges()
{
   QMutexLocker lock(&mMutex);
   auto localChanges = false;

   const auto commit = mCommitsMap.value(CommitInfo::ZERO_SHA, CommitInfo());

   if (commit.isValid())
   {
      const auto rf = revisionFile(CommitInfo::ZERO_SHA, commit.parent(0));
      localChanges = rf.count() - mUntrackedfiles.count() > 0;
   }

   return localChanges;
}

QVector<QPair<QString, QStringList>> GitCache::getBranches(References::Type type)
{
   QMutexLocker lock(&mMutex);
   QVector<QPair<QString, QStringList>> branches;

   for (const auto &ref : mReferences.toStdMap())
      branches.append(QPair<QString, QStringList>(ref.first, ref.second.getReferences(type)));

   return branches;
}

QMap<QString, QString> GitCache::getTags(References::Type tagType) const
{
   QMap<QString, QString> tags;

   if (tagType == References::Type::LocalTag)
   {
      for (auto &reference : mReferences.toStdMap())
      {
         const auto tagNames = reference.second.getReferences(tagType);

         for (const auto &tag : tagNames)
            tags[tag] = reference.first;
      }
   }
   else
      tags = mRemoteTags;

   return tags;
}

void GitCache::updateTags(const QMap<QString, QString> &remoteTags)
{
   mRemoteTags = remoteTags;

   emit signalCacheUpdated();
}

void GitCache::resetLanes(const CommitInfo &c, bool isFork)
{
   const auto nextSha = c.parentsCount() == 0 ? QString() : c.parent(0);

   mLanes.nextParent(nextSha);

   if (c.parentsCount() > 1)
      mLanes.afterMerge();
   if (isFork)
      mLanes.afterFork();
   if (mLanes.isBranch())
      mLanes.afterBranch();
}

bool GitCache::checkSha(const QString &originalSha, const QString &currentSha) const
{
   if (originalSha == currentSha)
      return true;

   if (const auto iter = mCommitsMap.find(currentSha); iter != mCommitsMap.cend())
      return checkSha(originalSha, iter->parent(0));

   return false;
}

int GitCache::commitCount() const
{
   return mCommits.count();
}

RevisionFiles GitCache::fakeWorkDirRevFile(const QString &diffIndex, const QString &diffIndexCache)
{
   RevisionFiles rf(diffIndex);
   rf.setOnlyModified(false);

   for (const auto &it : qAsConst(mUntrackedfiles))
   {
      rf.mFiles.append(it);
      rf.setStatus(RevisionFiles::UNKNOWN);
      rf.mergeParent.append(1);
   }

   RevisionFiles cachedFiles(diffIndexCache, true);

   for (auto i = 0; i < rf.count(); i++)
   {
      if (const auto cachedIndex = cachedFiles.mFiles.indexOf(rf.getFile(i)); cachedIndex != -1)
      {
         if (cachedFiles.statusCmp(cachedIndex, RevisionFiles::CONFLICT))
            rf.appendStatus(i, RevisionFiles::CONFLICT);
         else if (cachedFiles.statusCmp(cachedIndex, RevisionFiles::MODIFIED)
                  && cachedFiles.statusCmp(cachedIndex, RevisionFiles::IN_INDEX))
            rf.appendStatus(i, RevisionFiles::PARTIALLY_CACHED);
         else if (cachedFiles.statusCmp(cachedIndex, RevisionFiles::IN_INDEX))
            rf.appendStatus(i, RevisionFiles::IN_INDEX);
      }
   }

   return rf;
}

void GitCache::setUntrackedFilesList(const QVector<QString> &untrackedFiles)
{
   mUntrackedfiles = untrackedFiles;
}
