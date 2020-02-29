#include "GitMerge.h"

#include <GitBase.h>
#include <GitRepoLoader.h>
#include <QLogger.h>

using namespace QLogger;

GitMerge::GitMerge(const QSharedPointer<GitBase> &gitBase, QSharedPointer<RevisionsCache> cache)
   : mGitBase(gitBase)
   , mCache(cache)
{
}

GitExecResult GitMerge::merge(const QString &into, QStringList sources)
{
   QLog_Debug("Git", QString("Executing merge: {%1} into {%2}").arg(sources.join(","), into));

   const auto retCheckout = mGitBase->run(QString("git checkout -q %1").arg(into));

   if (!retCheckout.first)
      return retCheckout;

   const auto retMerge = mGitBase->run(QString("git merge -q ") + sources.join(" "));

   if (retMerge.first)
   {
      QScopedPointer<GitRepoLoader> gitLoader(new GitRepoLoader(mGitBase, mCache));
      gitLoader->updateWipRevision();
   }

   return retMerge;
}

GitExecResult GitMerge::abortMerge() const
{
   return mGitBase->run("git merge --abort");
}

GitExecResult GitMerge::applyMerge() const
{
   return mGitBase->run("git commit --no-edit");
}
