#include "GitMerge.h"

#include <GitBase.h>
#include <GitRepoLoader.h>

#include <QLogger.h>
#include <BenchmarkTool.h>

using namespace QLogger;
using namespace Benchmarker;

GitMerge::GitMerge(const QSharedPointer<GitBase> &gitBase, QSharedPointer<RevisionsCache> cache)
   : mGitBase(gitBase)
   , mCache(cache)
{
}

GitExecResult GitMerge::merge(const QString &into, QStringList sources)
{
   BenchmarkStart();

   QLog_Debug("Git", QString("Executing merge: {%1} into {%2}").arg(sources.join(","), into));

   const auto retCheckout = mGitBase->run(QString("git checkout -q %1").arg(into));

   if (!retCheckout.success)
   {
      BenchmarkEnd();
      return retCheckout;
   }

   const auto retMerge = mGitBase->run(QString("git merge -Xignore-all-space ") + sources.join(" "));

   if (retMerge.success)
   {
      QScopedPointer<GitRepoLoader> gitLoader(new GitRepoLoader(mGitBase, mCache));
      gitLoader->updateWipRevision();
   }

   BenchmarkEnd();

   return retMerge;
}

GitExecResult GitMerge::abortMerge() const
{
   BenchmarkStart();

   QLog_Debug("Git", QString("Merge aborted"));

   const auto ret = mGitBase->run("git merge --abort");

   BenchmarkEnd();

   return ret;
}

GitExecResult GitMerge::applyMerge() const
{
   BenchmarkStart();

   QLog_Debug("Git", QString("Merge commit"));

   const auto ret = mGitBase->run("git commit --no-edit");

   BenchmarkEnd();

   return ret;
}
