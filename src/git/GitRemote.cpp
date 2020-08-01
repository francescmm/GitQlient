#include "GitRemote.h"

#include <GitBase.h>
#include <GitConfig.h>
#include <GitQlientSettings.h>

#include <QLogger.h>
#include <BenchmarkTool.h>

using namespace QLogger;
using namespace Benchmarker;

GitRemote::GitRemote(const QSharedPointer<GitBase> &gitBase)
   : mGitBase(gitBase)
{
}

GitExecResult GitRemote::pushBranch(const QString &branchName, bool force)
{
   BenchmarkStart();

   QLog_Debug("Git", QString("Executing push"));

   QScopedPointer<GitConfig> gitConfig(new GitConfig(mGitBase));
   auto ret = gitConfig->getRemoteForBranch(branchName);

   if (ret.success)
   {
      const auto remote = ret.output.toString().isEmpty() ? QString("origin") : ret.output.toString();
      ret = mGitBase->run(QString("git push %1 %2 %3").arg(remote, branchName, force ? QString("--force") : QString()));
   }

   BenchmarkEnd();

   return ret;
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

   GitQlientSettings settings;
   const auto pruneOnFetch = settings.localValue(mGitBase->getGitQlientSettingsDir(), "PruneOnFetch", true).toBool();

   const auto cmd
       = QString("git fetch --all --tags --force").arg(pruneOnFetch ? QString("--prune --prune-tags") : QString());
   const auto ret = mGitBase->run(cmd).success;

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
