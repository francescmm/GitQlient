#include "GitRemote.h"

#include <GitBase.h>

#include <QLogger.h>
#include <QBenchmark.h>

using namespace QLogger;
using namespace QBenchmark;

GitRemote::GitRemote(const QSharedPointer<GitBase> &gitBase)
   : mGitBase(gitBase)
{
}

GitExecResult GitRemote::push(bool force)
{
   QBenchmarkStart();

   QLog_Debug("Git", QString("Executing push"));

   const auto ret = mGitBase->run(QString("git push ").append(force ? QString("--force") : QString()));

   QBenchmarkEnd();

   return ret;
}

GitExecResult GitRemote::pull()
{
   QBenchmarkStart();

   QLog_Debug("Git", QString("Executing pull"));

   const auto ret = mGitBase->run("git pull");

   QBenchmarkEnd();

   return ret;
}

bool GitRemote::fetch()
{
   QBenchmarkStart();

   QLog_Debug("Git", QString("Executing fetch with prune"));

   const auto ret = mGitBase->run("git fetch --all --tags --prune --force").success;

   QBenchmarkEnd();

   return ret;
}

GitExecResult GitRemote::prune()
{
   QBenchmarkStart();

   QLog_Debug("Git", QString("Executing prune"));

   const auto ret = mGitBase->run("git remote prune origin");

   QBenchmarkStart();

   return ret;
}
