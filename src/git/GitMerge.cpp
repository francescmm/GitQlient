#include "GitMerge.h"

#include <GitBase.h>
#include <GitRepoLoader.h>

#include <QLogger.h>
#include <QBenchmark.h>

using namespace QLogger;
using namespace QBenchmark;

GitMerge::GitMerge(const QSharedPointer<GitBase> &gitBase, QSharedPointer<RevisionsCache> cache)
   : mGitBase(gitBase)
   , mCache(cache)
{
}

GitExecResult GitMerge::merge(const QString &into, QStringList sources)
{
   QBenchmarkStart();

   QLog_Debug("Git", QString("Executing merge: {%1} into {%2}").arg(sources.join(","), into));

   const auto retCheckout = mGitBase->run(QString("git checkout -q %1").arg(into));

   if (!retCheckout.success)
   {
      QBenchmarkEnd();
      return retCheckout;
   }

   const auto retMerge = mGitBase->run(QString("git merge ") + sources.join(" "));

   if (retMerge.success)
   {
      QScopedPointer<GitRepoLoader> gitLoader(new GitRepoLoader(mGitBase, mCache));
      gitLoader->updateWipRevision();
   }

   QBenchmarkEnd();

   return retMerge;
}

GitExecResult GitMerge::abortMerge() const
{
   QBenchmarkStart();

   QLog_Debug("Git", QString("Merge aborted"));

   const auto ret = mGitBase->run("git merge --abort");

   QBenchmarkEnd();

   return ret;
}

GitExecResult GitMerge::applyMerge() const
{
   QBenchmarkStart();

   QLog_Debug("Git", QString("Merge commit"));

   const auto ret = mGitBase->run("git commit --no-edit");

   QBenchmarkEnd();

   return ret;
}
