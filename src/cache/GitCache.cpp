#include "GitCache.h"

#include <QLogger.h>
#include <WipRevisionInfo.h>

using namespace QLogger;

GitCache::GitCache(QObject *parent)
   : QObject(parent)
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
   , mCommitsMutex(QMutex::Recursive)
   , mRevisionsMutex(QMutex::Recursive)
   , mReferencesMutex(QMutex::Recursive)
#endif
{
}

GitCache::~GitCache()
{
   clearInternalData();
}

void GitCache::setup(const QString &parentSha, const RevisionFiles &files, QVector<CommitInfo> commits)
{
   QMutexLocker lock(&mCommitsMutex);

   mCommitsCache = std::move(commits);

   mInitialized = true;
   mConfigured = false;

   mCommitsMap.clear();
   mCommitsMap.squeeze();
   mUntrackedFiles.clear();
   mUntrackedFiles.squeeze();
   mLanes.clear();

   QLog_Debug("Cache", QString("Adding WIP revision."));

   insertWipRevision(parentSha, files);

   const auto totalCommits = mCommitsCache.count();

   QLog_Debug("Cache", QString("Configuring the cache for {%1} elements.").arg(totalCommits));

   mCommitsMap.reserve(totalCommits);

   QLog_Debug("Cache", QString("Adding committed revisions."));

   QHash<QString, QVector<CommitInfo *>> tmpChildsStorage;

   for (auto iter = mCommitsCache.begin() + 1; iter != mCommitsCache.end(); ++iter)
   {
      auto &commit = *iter;
      calculateLanes(commit);

      if (commit.sha == mCommitsCache[0].firstParent())
         commit.appendChild(&mCommitsCache[0]);

      mCommitsMap[commit.sha] = &commit;

      if (tmpChildsStorage.contains(commit.sha))
      {
         for (const auto &child : std::as_const(tmpChildsStorage[commit.sha]))
            commit.appendChild(child);

         tmpChildsStorage.remove(commit.sha);
      }

      for (const auto &parent : std::as_const(commit.mParentsSha))
         tmpChildsStorage[parent].append(&commit);
   }

   tmpChildsStorage.clear();
   tmpChildsStorage.squeeze();
}

CommitInfo GitCache::commitInfo(int row)
{
   QMutexLocker lock(&mCommitsMutex);

   return row >= 0 && row < mCommitsCache.count() ? mCommitsCache.at(row) : CommitInfo();
}

auto GitCache::searchCommit(const QString &text, const int startingPoint) const
{
   return std::find_if(mCommitsCache.constBegin() + startingPoint, mCommitsCache.constEnd(),
                       [text](const CommitInfo &info) { return info.contains(text); });
}

auto GitCache::reverseSearchCommit(const QString &text, int startingPoint) const
{
   const auto startEndPos = startingPoint > 0 ? mCommitsCache.count() - startingPoint + 1 : 0;

   return std::find_if(mCommitsCache.crbegin() + startEndPos, mCommitsCache.crend(),
                       [text](const CommitInfo &info) { return info.contains(text); });
}

CommitInfo GitCache::searchCommitInfo(const QString &text, int startingPoint, bool reverse)
{
   QMutexLocker lock(&mCommitsMutex);
   CommitInfo commit;

   if (!reverse)
   {
      auto commitIter = searchCommit(text, startingPoint);

      if (commitIter == mCommitsCache.constEnd())
         commitIter = searchCommit(text);

      if (commitIter != mCommitsCache.constEnd())
         commit = *commitIter;
   }
   else
   {
      auto commitIter = reverseSearchCommit(text, startingPoint);

      if (commitIter == mCommitsCache.crend())
         commitIter = reverseSearchCommit(text);

      if (commitIter != mCommitsCache.crend())
         commit = *commitIter;
   }

   return commit;
}

CommitInfo GitCache::commitInfo(const QString &sha)
{
   QMutexLocker lock(&mCommitsMutex);

   if (!sha.isEmpty())
   {
      const auto c = mCommitsMap.value(sha, nullptr);

      if (!c)
      {
         const auto shas = mCommitsMap.keys();
         const auto it = std::find_if(shas.cbegin(), shas.cend(),
                                      [sha](const QString &shaToCompare) { return shaToCompare.startsWith(sha); });

         if (it != shas.cend())
            return *mCommitsMap.value(*it);

         return CommitInfo();
      }

      return *c;
   }

   return CommitInfo();
}

std::optional<RevisionFiles> GitCache::revisionFile(const QString &sha1, const QString &sha2) const
{
   QMutexLocker lock(&mRevisionsMutex);

   const auto iter = mRevisionFilesMap.constFind(qMakePair(sha1, sha2));

   if (iter != mRevisionFilesMap.cend())
      return *iter;

   return std::nullopt;
}

void GitCache::clearReferences()
{
   QMutexLocker lock(&mReferencesMutex);
   mReferences.clear();
   mReferences.squeeze();
}

void GitCache::insertWipRevision(const QString parentSha, const RevisionFiles &files)
{
   auto newParentSha = parentSha;

   QLog_Debug("Cache", QString("Updating the WIP commit. The actual parent has SHA {%1}.").arg(newParentSha));

   insertRevisionFile(ZERO_SHA, newParentSha, files);

   QStringList parents;

   if (!newParentSha.isEmpty())
      parents.append(newParentSha);

   if (mLanes.isEmpty())
      mLanes.init(ZERO_SHA);

   const auto log = files.count() == mUntrackedFiles.count() ? tr("No local changes") : tr("Local changes");
   CommitInfo c(ZERO_SHA, parents, std::chrono::seconds(QDateTime::currentSecsSinceEpoch()), log);
   calculateLanes(c);

   if (!mCommitsCache.isEmpty())
      c.setLanes(mCommitsCache[0].lanes());

   if (mCommitsCache[0].sha != ZERO_SHA)
      mCommitsCache.prepend(std::move(c));
   else
      mCommitsCache[0] = std::move(c);

   mCommitsMap.insert(ZERO_SHA, &mCommitsCache[0]);
}

bool GitCache::insertRevisionFiles(const QString &sha1, const QString &sha2, const RevisionFiles &file)
{
   QMutexLocker lock(&mRevisionsMutex);

   return insertRevisionFile(sha1, sha2, file);
}

bool GitCache::insertRevisionFile(const QString &sha1, const QString &sha2, const RevisionFiles &file)
{
   const auto key = qMakePair(sha1, sha2);
   const auto emptyShas = !sha1.isEmpty() && !sha2.isEmpty();
   const auto isWip = sha1 == ZERO_SHA;

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
   QMutexLocker lock(&mReferencesMutex);

   QLog_Trace("Cache", QString("Adding a new reference with SHA {%1}.").arg(sha));

   mReferences[sha].addReference(type, reference);
}

void GitCache::deleteReference(const QString &sha, References::Type type, const QString &reference)
{
   QMutexLocker lock(&mReferencesMutex);

   mReferences[sha].removeReference(type, reference);
}

bool GitCache::hasReferences(const QString &sha)
{
   QMutexLocker lock(&mReferencesMutex);

   return mReferences.contains(sha) && !mReferences.value(sha).isEmpty();
}

QStringList GitCache::getReferences(const QString &sha, References::Type type)
{
   QMutexLocker lock(&mReferencesMutex);

   return mReferences.value(sha).getReferences(type);
}

QString GitCache::getShaOfReference(const QString &referenceName, References::Type type) const
{
   QMutexLocker lock(&mReferencesMutex);

   for (auto iter = mReferences.cbegin(); iter != mReferences.cend(); ++iter)
   {
      const auto references = iter.value().getReferences(type);

      for (const auto &reference : references)
         if (reference == referenceName)
            return iter.key();
   }

   return QString();
}

void GitCache::reloadCurrentBranchInfo(const QString &currentBranch, const QString &currentSha)
{
   QMutexLocker lock(&mReferencesMutex);

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

   if (!currentBranch.isEmpty())
      mReferences[currentSha].addReference(References::Type::LocalBranch, currentBranch);
}

bool GitCache::updateWipCommit(const QString &parentSha, const RevisionFiles &files)
{
   QMutexLocker lock(&mRevisionsMutex);
   QMutexLocker lock2(&mCommitsMutex);

   if (mConfigured)
   {
      insertWipRevision(parentSha, files);
      return true;
   }

   return false;
}

void GitCache::insertCommit(CommitInfo commit)
{
   QMutexLocker lock2(&mCommitsMutex);

   const auto sha = commit.sha;
   const auto parentSha = commit.firstParent();

   commit.setLanes({ LaneType::ACTIVE });

   mCommitsCache[0].setParents({ sha });
   mCommitsCache.insert(1, std::move(commit));
   mCommitsCache[1].appendChild(&mCommitsCache[0]);

   mCommitsMap.clear();

   const auto totalCommits = mCommitsCache.count();
   mCommitsMap.reserve(totalCommits);

   auto count = 0;
   for (auto &commit : mCommitsCache)
   {
      commit.pos = count;
      mCommitsMap[commit.sha] = &commit;
   }

   mCommitsMap[parentSha]->removeChild(mCommitsMap[ZERO_SHA]);
   mCommitsMap[parentSha]->appendChild(mCommitsMap[sha]);
}

void GitCache::updateCommit(const QString &oldSha, CommitInfo newCommit)
{
   QMutexLocker lock(&mCommitsMutex);
   QMutexLocker lock2(&mRevisionsMutex);

   auto &oldCommit = mCommitsMap[oldSha];
   const auto oldCommitParens = oldCommit->parents();
   const auto newCommitSha = newCommit.sha;
   const auto newPos = newCommit.pos;

   mCommitsCache[newPos] = std::move(newCommit);
   mCommitsMap.remove(oldSha);
   mCommitsMap.insert(newCommitSha, &mCommitsCache[newPos]);

   for (const auto &parent : oldCommitParens)
   {
      mCommitsMap[parent]->removeChild(oldCommit);
      mCommitsMap[parent]->appendChild(mCommitsMap[newCommitSha]);
   }

   const auto tags = getReferences(oldSha, References::Type::LocalTag);
   for (const auto &tag : tags)
   {
      insertReference(newCommitSha, References::Type::LocalTag, tag);
      deleteReference(oldSha, References::Type::LocalTag, tag);
   }

   const auto localBranches = getReferences(oldSha, References::Type::LocalBranch);
   for (const auto &branch : localBranches)
   {
      insertReference(newCommitSha, References::Type::LocalBranch, branch);
      deleteReference(oldSha, References::Type::LocalBranch, branch);
   }
}

void GitCache::calculateLanes(CommitInfo &c)
{
   const auto sha = c.sha;

   QLog_Trace("Cache", QString("Updating the lanes for SHA {%1}.").arg(sha));

   bool isDiscontinuity;
   bool isFork = mLanes.isFork(sha, isDiscontinuity);
   bool isMerge = c.parentsCount() > 1;

   if (isDiscontinuity)
      mLanes.changeActiveLane(sha);

   if (isFork)
      mLanes.setFork(sha);
   if (isMerge)
      mLanes.setMerge(c.parents());
   if (c.parentsCount() == 0)
      mLanes.setInitial();

   const auto lanes = mLanes.getLanes();

   resetLanes(c, isFork);

   c.setLanes(std::move(lanes));
}

bool GitCache::pendingLocalChanges()
{
   QMutexLocker lock(&mCommitsMutex);

   auto localChanges = false;

   if (const auto commit = mCommitsMap.value(ZERO_SHA, nullptr))
   {
      if (const auto rf = revisionFile(ZERO_SHA, commit->firstParent()); rf)
         localChanges = rf.value().count() - mUntrackedFiles.count() > 0;
   }

   return localChanges;
}

QVector<QPair<QString, QStringList>> GitCache::getBranches(References::Type type)
{
   QMutexLocker lock(&mReferencesMutex);
   QVector<QPair<QString, QStringList>> branches;

   for (auto iter = mReferences.cbegin(); iter != mReferences.cend(); ++iter)
      branches.append(QPair<QString, QStringList>(iter.key(), iter.value().getReferences(type)));

   return branches;
}

QMap<QString, QString> GitCache::getTags(References::Type tagType) const
{
   QMutexLocker lock(&mReferencesMutex);

   QMap<QString, QString> tags;

   for (auto iter = mReferences.cbegin(); iter != mReferences.cend(); ++iter)
   {
      const auto tagNames = iter->getReferences(tagType);

      for (const auto &tag : tagNames)
         tags[tag] = iter.key();
   }

   return tags;
}

void GitCache::updateTags(QMap<QString, QString> remoteTags)
{
   const auto end = remoteTags.cend();

   for (auto iter = remoteTags.cbegin(); iter != end; ++iter)
      insertReference(iter.value(), References::Type::RemoteTag, iter.key());

   emit signalCacheUpdated();
}

void GitCache::resetLanes(const CommitInfo &c, bool isFork)
{
   const auto nextSha = c.parentsCount() == 0 ? QString() : c.firstParent();

   mLanes.nextParent(nextSha);

   if (c.parentsCount() > 1)
      mLanes.afterMerge();
   if (isFork)
      mLanes.afterFork();
   if (mLanes.isBranch())
      mLanes.afterBranch();
}

void GitCache::clearInternalData()
{
   mCommitsMap.clear();
   mCommitsMap.squeeze();
   mReferences.clear();
   mRevisionFilesMap.clear();
   mRevisionFilesMap.squeeze();
   mUntrackedFiles.clear();
   mUntrackedFiles.squeeze();
   mLanes.clear();
   mReferences.clear();
   mReferences.squeeze();
}

int GitCache::commitCount() const
{
   return mCommitsCache.count();
}

void GitCache::setUntrackedFilesList(QVector<QString> untrackedFiles)
{
   mUntrackedFiles.clear();
   mUntrackedFiles.squeeze();
   mUntrackedFiles = std::move(untrackedFiles);
}
