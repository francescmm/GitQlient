#include "GitRemote.h"

#include <GitBase.h>
#include <QLogger.h>

using namespace QLogger;

GitRemote::GitRemote(const QSharedPointer<GitBase> &gitBase)
   : mGitBase(gitBase)
{
}

GitExecResult GitRemote::push(bool force)
{
   QLog_Debug("Git", QString("Executing push"));

   return mGitBase->run(QString("git push ").append(force ? QString("--force") : QString()));
}

GitExecResult GitRemote::pull()
{
   QLog_Debug("Git", QString("Executing pull"));

   return mGitBase->run("git pull");
}

bool GitRemote::fetch()
{
   QLog_Debug("Git", QString("Executing fetch with prune"));

   return mGitBase->run("git fetch --all --tags --prune --force").first;
}

GitExecResult GitRemote::prune()
{
   QLog_Debug("Git", QString("Executing prune"));

   return mGitBase->run("git remote prune origin");
}

GitExecResult GitRemote::merge(const QString &into, QStringList sources)
{
   QLog_Debug("Git", QString("Executing merge: {%1} into {%2}").arg(sources.join(","), into));

   const auto ret = mGitBase->run(QString("git checkout -q %1").arg(into));

   if (!ret.first)
      return ret;

   return mGitBase->run(QString("git merge -q ") + sources.join(" "));
}

GitExecResult GitRemote::abortMerge() const
{
   return mGitBase->run("git merge --abort");
}
