#include "GitWip.h"

#include <GitBase.h>
#include <GitCache.h>

#include <QLogger.h>

#include <QFile>

using namespace QLogger;

GitWip::GitWip(const QSharedPointer<GitBase> &git, const QSharedPointer<GitCache> &cache)
   : mGit(git)
   , mCache(cache)
{
}

QVector<QString> GitWip::getUntrackedFiles() const
{
   QLog_Debug("Git", QString("Executing getUntrackedFiles."));

   auto runCmd = QString("git ls-files --others --exclude-standard");

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
   const auto ret = mGit->run(runCmd).output.split('\n', Qt::SkipEmptyParts).toVector();
#else
   const auto ret = mGit->run(runCmd).output.split('\n', QString::SkipEmptyParts).toVector();
#endif

   return ret;
}

std::optional<QPair<QString, RevisionFiles>> GitWip::getWipInfo() const
{
   QLog_Debug("Git", QString("Executing processWip."));

   const auto ret = mGit->run("git rev-parse --revs-only HEAD");

   if (ret.success)
   {
      QString diffIndex;
      QString diffIndexCached;

      auto parentSha = ret.output.trimmed();

      if (parentSha.isEmpty())
         parentSha = CommitInfo::INIT_SHA;

      const auto ret3 = mGit->run(QString("git diff-index %1").arg(parentSha));
      diffIndex = ret3.success ? ret3.output : QString();

      const auto ret4 = mGit->run(QString("git diff-index --cached %1").arg(parentSha));
      diffIndexCached = ret4.success ? ret4.output : QString();

      auto files = fakeWorkDirRevFile(diffIndex, diffIndexCached);

      return qMakePair(parentSha, std::move(files));
   }

   return std::nullopt;
}

std::optional<GitWip::FileStatus> GitWip::getFileStatus(const QString &filePath) const
{
   QLog_Debug("Git", QString("Getting file status."));

   const auto ret = mGit->run(QString("git diff-files -c %1").arg(filePath));

   if (ret.success)
   {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
      const auto lines = ret.output.split("\n", Qt::SkipEmptyParts);
#else
      const auto lines = ret.output.split("\n", QString::SkipEmptyParts);
#endif

      if (lines.count() > 1)
         return FileStatus::DeletedByThem;
      else
      {
         const auto statusField = lines[0].split(" ").last().split("\t").constFirst();

         if (statusField.count() == 2)
            return FileStatus::BothModified;

         return FileStatus::DeletedByUs;
      }
   }

   return std::nullopt;
}

bool GitWip::updateWip() const
{
   const auto files = getUntrackedFiles();
   mCache->setUntrackedFilesList(std::move(files));

   if (const auto info = getWipInfo())
   {
      return mCache->updateWipCommit(info->first, info->second);
   }

   return false;
}

RevisionFiles GitWip::fakeWorkDirRevFile(const QString &diffIndex, const QString &diffIndexCache) const
{
   RevisionFiles rf(diffIndex);
   rf.setOnlyModified(false);

   for (const auto &it : mCache->getUntrackedFiles())
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
         {
            rf.appendStatus(i, RevisionFiles::CONFLICT);

            const auto status = getFileStatus(rf.getFile(i));

            switch (status.value_or(GitWip::FileStatus::BothModified))
            {
               case GitWip::FileStatus::DeletedByThem:
               case GitWip::FileStatus::DeletedByUs:
                  rf.appendStatus(i, RevisionFiles::DELETED);
                  break;
               default:
                  break;
            }
         }
         else if (cachedFiles.statusCmp(cachedIndex, RevisionFiles::MODIFIED)
                  && cachedFiles.statusCmp(cachedIndex, RevisionFiles::IN_INDEX))
            rf.appendStatus(i, RevisionFiles::PARTIALLY_CACHED);
         else if (cachedFiles.statusCmp(cachedIndex, RevisionFiles::IN_INDEX))
            rf.appendStatus(i, RevisionFiles::IN_INDEX);
      }
   }

   return rf;
}
