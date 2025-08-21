#include <CommitInfo.h>
#include <GraphCache.h>

void GraphCache::clear()
{
   mLanes.clear();
   mShaToLanes.clear();
   mPositionToSha.clear();
}

void GraphCache::init(const QString &sha)
{
   mLanes.init(sha);
}

void GraphCache::processLanes(std::span<CommitInfo> commits)
{
   for (auto &commit : commits)
      calculateLanes(commit);
}

void GraphCache::addCommit(const CommitInfo &commit)
{
   mShaToLanes[commit.sha] = { { { LaneType::ACTIVE } }, 0 };
   mPositionToSha[commit.pos] = commit.sha;
}

void GraphCache::updateCommitLanes(CommitInfo &commit)
{
   calculateLanes(commit);
}

void GraphCache::calculateLanes(CommitInfo &commit)
{
   const auto sha = commit.sha;
   bool isDiscontinuity;
   const auto isFork = mLanes.isFork(sha, isDiscontinuity);
   const auto isMerge = commit.parentsCount() > 1;

   if (isDiscontinuity)
      mLanes.changeActiveLane(sha);

   if (isFork)
      mLanes.setFork(sha);
   if (isMerge)
      mLanes.setMerge(commit.parents());
   if (commit.parentsCount() == 0)
      mLanes.setInitial();

   const auto lanes = mLanes.getLanes();
   const auto activeLane = mLanes.getActiveLane();

   resetLanes(commit, isFork);

   mShaToLanes[sha] = { lanes, activeLane };
   mPositionToSha[commit.pos] = sha;
}

QVector<Lane> GraphCache::getLanes(const QString &sha) const
{
   return mShaToLanes.value(sha).lanes;
}

QVector<Lane> GraphCache::getLanes(int position) const
{
   if (const auto sha = mPositionToSha.value(position); !sha.isEmpty())
      return mShaToLanes.value(sha).lanes;
   return {};
}

int GraphCache::getLanesCount(const QString &sha) const
{
   return mShaToLanes.value(sha).lanes.count();
}

Lane GraphCache::getLaneAt(const QString &sha, int index) const
{
   const auto &laneData = mShaToLanes.value(sha);
   if (index >= 0 && index < laneData.lanes.count())
      return laneData.lanes.at(index);
   return Lane();
}

int GraphCache::getActiveLane(const QString &sha) const
{
   return mShaToLanes.value(sha).activeLane;
}

void GraphCache::resetLanes(const CommitInfo &commit, bool isFork)
{
   const auto nextSha = commit.parentsCount() == 0 ? QString() : commit.firstParent();

   mLanes.nextParent(nextSha);

   if (commit.parentsCount() > 1)
      mLanes.afterMerge();
   if (isFork)
      mLanes.afterFork();
   if (mLanes.isBranch())
      mLanes.afterBranch();
}
