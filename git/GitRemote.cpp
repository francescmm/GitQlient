#include "GitRemote.h"

#include <GitBase.h>

GitRemote::GitRemote(const QSharedPointer<GitBase> &gitBase)
   : mGitBase(gitBase)
{
}

GitExecResult GitRemote::push(bool force)
{
   return mGitBase->run(QString("git push ").append(force ? QString("--force") : QString()));
}

GitExecResult GitRemote::pull()
{
   return mGitBase->run("git pull");
}

bool GitRemote::fetch()
{
   return mGitBase->run("git fetch --all --tags --prune --force").first;
}

GitExecResult GitRemote::prune()
{
   return mGitBase->run("git remote prune origin");
}

GitExecResult GitRemote::merge(const QString &into, QStringList sources)
{
   const auto ret = mGitBase->run(QString("git checkout -q %1").arg(into));

   if (!ret.first)
      return ret;

   return mGitBase->run(QString("git merge -q ") + sources.join(" "));
}
