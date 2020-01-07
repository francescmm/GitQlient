#include "RevisionsCache.h"

RevisionsCache::RevisionsCache(QObject *parent)
   : QObject(parent)
{
}

void RevisionsCache::configure(int numElementsToStore)
{
   if (mCacheData.mCommits.isEmpty())
   {
      // We reserve 1 extra slots for the ZERO_SHA (aka WIP commit)
      mCacheData.mCommits.resize(numElementsToStore + 1);
      mCacheData.revs.reserve(numElementsToStore + 1);
   }

   mCacheData.mCacheLocked = false;
}

CommitInfo RevisionsCache::getCommitInfoByRow(int row) const
{
   const auto commit = row >= 0 && row < mCacheData.mCommits.count() ? mCacheData.mCommits.at(row) : nullptr;

   return commit ? *commit : CommitInfo();
}

CommitInfo RevisionsCache::getCommitInfo(const QString &sha) const
{
   if (!sha.isEmpty())
   {
      CommitInfo *c;

      c = mCacheData.revs.value(sha, nullptr);

      if (c == nullptr)
      {
         const auto shas = mCacheData.revs.keys();
         const auto it = std::find_if(shas.cbegin(), shas.cend(),
                                      [sha](const QString &shaToCompare) { return shaToCompare.startsWith(sha); });

         if (it != shas.cend())
            return *mCacheData.revs.value(*it);
      }
      else
         return *c;
   }

   return CommitInfo();
}

void RevisionsCache::insertCommitInfo(CommitInfo rev)
{
   if (!mCacheData.mCacheLocked && !mCacheData.revs.contains(rev.sha()))
   {
      updateLanes(rev, mCacheData.lns);

      const auto commit = new CommitInfo(rev);

      if (rev.orderIdx >= mCacheData.mCommits.count())
         mCacheData.mCommits.insert(rev.orderIdx, commit);
      else if (!(mCacheData.mCommits[rev.orderIdx] && *mCacheData.mCommits[rev.orderIdx] == *commit))
      {
         delete mCacheData.mCommits[rev.orderIdx];
         mCacheData.mCommits[rev.orderIdx] = commit;
      }

      mCacheData.revs.insert(rev.sha(), commit);

      if (mCacheData.revs.contains(rev.parent(0)))
         mCacheData.revs.remove(rev.parent(0));
   }
}

void RevisionsCache::insertReference(const QString &sha, Reference ref)
{
   mCacheData.mRefsShaMap[sha] = std::move(ref);
}

void RevisionsCache::updateWipCommit(const QString &parentSha, const QString &diffIndex, const QString &diffIndexCache)
{
   const auto fakeRevFile = fakeWorkDirRevFile(diffIndex, diffIndexCache);

   insertRevisionFile(ZERO_SHA, fakeRevFile);

   if (!mCacheData.mCacheLocked)
   {
      QString longLog;
      const auto author = QString("-");
      const auto log = fakeRevFile.count() == mCacheData.mUntrackedfiles.count() ? QString("No local changes")
                                                                                 : QString("Local changes");
      CommitInfo c(ZERO_SHA, { parentSha }, author, QDateTime::currentDateTime().toSecsSinceEpoch(), log, longLog, 0);
      c.isDiffCache = true;

      updateLanes(c, mCacheData.lns);

      if (mCacheData.mCommits[c.orderIdx])
         c.lanes = mCacheData.mCommits[c.orderIdx]->lanes;

      const auto sha = c.sha();
      const auto commit = new CommitInfo(std::move(c));

      delete mCacheData.mCommits[commit->orderIdx];
      mCacheData.mCommits[commit->orderIdx] = commit;

      mCacheData.revs.insert(sha, commit);
   }
}

void RevisionsCache::updateLanes(CommitInfo &c, Lanes &lns)
{
   const auto sha = c.sha();

   if (lns.isEmpty())
      lns.init(c.sha());

   bool isDiscontinuity;
   bool isFork = lns.isFork(sha, isDiscontinuity);
   bool isMerge = (c.parentsCount() > 1);
   bool isInitial = (c.parentsCount() == 0);

   if (isDiscontinuity)
      lns.changeActiveLane(sha); // uses previous isBoundary state

   lns.setBoundary(c.isBoundary()); // update must be here

   if (isFork)
      lns.setFork(sha);
   if (isMerge)
      lns.setMerge(c.parents());
   if (isInitial)
      lns.setInitial();

   lns.setLanes(c.lanes); // here lanes are snapshotted

   const QString &nextSha = (isInitial) ? "" : QString(c.parent(0));

   lns.nextParent(nextSha);

   if (isMerge)
      lns.afterMerge();
   if (isFork)
      lns.afterFork();
   if (lns.isBranch())
      lns.afterBranch();
}

RevisionFile RevisionsCache::parseDiffFormat(const QString &buf, FileNamesLoader &fl)
{
   RevisionFile rf;
   auto parNum = 1;
   const auto lines = buf.split("\n", QString::SkipEmptyParts);

   for (auto line : lines)
   {
      if (line[0] == ':') // avoid sha's in merges output
      {
         if (line[1] == ':')
         { // it's a combined merge
            /* For combined merges rename/copy information is useless
             * because nor the original file name, nor similarity info
             * is given, just the status tracks that in the left/right
             * branch a renamed/copy occurred (as example status could
             * be RM or MR). For visualization purposes we could consider
             * the file as modified
             */
            if (fl.rf != &rf)
            {
               flushFileNames(fl);
               fl.rf = &rf;
            }
            appendFileName(line.section('\t', -1), fl);
            rf.setStatus("M");
            rf.mergeParent.append(parNum);
         }
         else
         {
            if (line.at(98) == '\t') // Faster parsing in normal case
            {
               if (fl.rf != &rf)
               {
                  flushFileNames(fl);
                  fl.rf = &rf;
               }
               appendFileName(line.mid(99), fl);
               rf.setStatus(line.at(97));
               rf.mergeParent.append(parNum);
            }
            else // It's a rename or a copy, we are not in fast path now!
               setExtStatus(rf, line.mid(97), parNum, fl);
         }
      }
      else
         ++parNum;
   }

   return rf;
}

void RevisionsCache::appendFileName(const QString &name, FileNamesLoader &fl)
{
   int idx = name.lastIndexOf('/') + 1;
   const QString &dr = name.left(idx);
   const QString &nm = name.mid(idx);

   auto it = mCacheData.mDirNames.indexOf(dr);
   if (it == -1)
   {
      int idx = mCacheData.mDirNames.count();
      mCacheData.mDirNames.append(dr);
      fl.rfDirs.append(idx);
   }
   else
      fl.rfDirs.append(it);

   it = mCacheData.mFileNames.indexOf(nm);
   if (it == -1)
   {
      int idx = mCacheData.mFileNames.count();
      mCacheData.mFileNames.append(nm);
      fl.rfNames.append(idx);
   }
   else
      fl.rfNames.append(it);

   fl.files.append(name);
}

void RevisionsCache::flushFileNames(FileNamesLoader &fl)
{
   if (!fl.rf)
      return;

   for (auto i = 0; i < fl.rfNames.count(); ++i)
   {
      const auto dirName = mCacheData.mDirNames.at(fl.rfDirs.at(i));
      const auto fileName = mCacheData.mFileNames.at(fl.rfNames.at(i));

      if (!fl.rf->mFiles.contains(dirName + fileName))
         fl.rf->mFiles.append(dirName + fileName);
   }

   fl.rfNames.clear();
   fl.rf = nullptr;
}

int RevisionsCache::findFileIndex(const RevisionFile &rf, const QString &name)
{
   if (name.isEmpty())
      return -1;

   const auto idx = name.lastIndexOf('/') + 1;
   const auto dr = name.left(idx);
   const auto nm = name.mid(idx);

   // return rf.mFiles.indexOf(name);
   const auto found = rf.mFiles.indexOf(name);
   return found;
}

bool RevisionsCache::pendingLocalChanges() const
{
   const auto rf = getRevisionFile(ZERO_SHA);
   return mCacheData.mRevsFiles.value(ZERO_SHA).count() == mCacheData.mUntrackedfiles.count();
}

void RevisionsCache::setExtStatus(RevisionFile &rf, const QString &rowSt, int parNum, FileNamesLoader &fl)
{
   const QStringList sl(rowSt.split('\t', QString::SkipEmptyParts));
   if (sl.count() != 3)
      return;

   // we want store extra info with format "orig --> dest (Rxx%)"
   // but git give us something like "Rxx\t<orig>\t<dest>"
   QString type = sl[0];
   type.remove(0, 1);
   const QString &orig = sl[1];
   const QString &dest = sl[2];
   const QString extStatusInfo(orig + " --> " + dest + " (" + QString::number(type.toInt()) + "%)");

   /*
    NOTE: we set rf.extStatus size equal to position of latest
          copied/renamed file. So it can have size lower then
          rf.count() if after copied/renamed file there are
          others. Here we have no possibility to know final
          dimension of this RefFile. We are still in parsing.
 */

   // simulate new file
   if (fl.rf != &rf)
   {
      flushFileNames(fl);
      fl.rf = &rf;
   }
   appendFileName(dest, fl);
   rf.mergeParent.append(parNum);
   rf.setStatus(RevisionFile::NEW);
   rf.appendExtStatus(extStatusInfo);

   // simulate deleted orig file only in case of rename
   if (type.at(0) == 'R')
   { // renamed file
      if (fl.rf != &rf)
      {
         flushFileNames(fl);
         fl.rf = &rf;
      }
      appendFileName(orig, fl);
      rf.mergeParent.append(parNum);
      rf.setStatus(RevisionFile::DELETED);
      rf.appendExtStatus(extStatusInfo);
   }
   rf.setOnlyModified(false);
}

void RevisionsCache::clear()
{
   mCacheData.mCacheLocked = true;
   mCacheData.mDirNames.clear();
   mCacheData.mFileNames.clear();
   mCacheData.mRevsFiles.clear();
   mCacheData.mRefsShaMap.clear();
   mCacheData.lns.clear();
   mCacheData.revs.clear();
}

RevisionFile RevisionsCache::fakeWorkDirRevFile(const QString &diffIndex, const QString &diffIndexCache)
{
   FileNamesLoader fl;
   RevisionFile rf = parseDiffFormat(diffIndex, fl);
   rf.setOnlyModified(false);

   for (auto it : qAsConst(mCacheData.mUntrackedfiles))
   {
      if (fl.rf != &rf)
      {
         flushFileNames(fl);
         fl.rf = &rf;
      }

      appendFileName(it, fl);
      rf.setStatus(RevisionFile::UNKNOWN);
      rf.mergeParent.append(1);
   }

   RevisionFile cachedFiles = parseDiffFormat(diffIndexCache, fl);
   flushFileNames(fl);

   for (auto i = 0; i < rf.count(); i++)
   {
      if (findFileIndex(cachedFiles, rf.getFile(i)) != -1)
      {
         if (cachedFiles.statusCmp(i, RevisionFile::CONFLICT))
            rf.appendStatus(i, RevisionFile::CONFLICT);

         rf.appendStatus(i, RevisionFile::IN_INDEX);
      }
   }

   return rf;
}

RevisionFile RevisionsCache::parseDiff(const QString &sha, const QString &logDiff)
{
   FileNamesLoader fl;

   RevisionFile rf = parseDiffFormat(logDiff, fl);
   flushFileNames(fl);

   insertRevisionFile(sha, rf);

   return rf;
}
