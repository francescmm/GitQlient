#include "GitRemote.h"

#include <GitBase.h>

#include <QLogger.h>
#include <BenchmarkTool.h>

using namespace QLogger;
using namespace GitQlientTools;

GitRemote::GitRemote(const QSharedPointer<GitBase> &gitBase)
   : mGitBase(gitBase)
{
}

GitExecResult GitRemote::push(bool force)
{
   BenchmarkStart();

   QLog_Debug("Git", QString("Executing push"));

   const auto ret = mGitBase->run(QString("git push ").append(force ? QString("--force") : QString()));

   BenchmarkEnd();

   return ret;
}

GitExecResult GitRemote::pull()
{
   BenchmarkStart();

   QLog_Debug("Git", QString("Executing pull"));

   const auto ret = mGitBase->run("git pull");

   BenchmarkEnd();

   return ret;
}

bool GitRemote::fetch()
{
   BenchmarkStart();

   QLog_Debug("Git", QString("Executing fetch with prune"));

   const auto ret = mGitBase->run("git fetch --all --tags --prune --force").success;

   BenchmarkEnd();

   return ret;
}

GitExecResult GitRemote::prune()
{
   BenchmarkStart();

   QLog_Debug("Git", QString("Executing prune"));

   const auto ret = mGitBase->run("git remote prune origin");

   BenchmarkStart();

   return ret;
}
