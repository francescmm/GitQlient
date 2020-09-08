#include "GitRemote.h"

#include <GitBase.h>
#include <GitConfig.h>
#include <GitSubmodules.h>
#include <GitQlientSettings.h>

#include <QLogger.h>

using namespace QLogger;

GitRemote::GitRemote(const QSharedPointer<GitBase> &gitBase)
   : mGitBase(gitBase)
{
}

GitExecResult GitRemote::pushBranch(const QString &branchName, bool force)
{

   QLog_Debug("Git", QString("Executing push"));

   QScopedPointer<GitConfig> gitConfig(new GitConfig(mGitBase));
   auto ret = gitConfig->getRemoteForBranch(branchName);

   if (ret.success)
   {
      const auto remote = ret.output.toString().isEmpty() ? QString("origin") : ret.output.toString();
      ret = mGitBase->run(QString("git push %1 %2 %3").arg(remote, branchName, force ? QString("--force") : QString()));
   }

   return ret;
}

GitExecResult GitRemote::push(bool force)
{

   QLog_Debug("Git", QString("Executing push"));

   const auto ret = mGitBase->run(QString("git push ").append(force ? QString("--force") : QString()));

   return ret;
}

GitExecResult GitRemote::pull()
{

   QLog_Debug("Git", QString("Executing pull"));

   auto ret = mGitBase->run("git pull");

   GitQlientSettings settings;
   const auto updateOnPull = settings.localValue(mGitBase->getGitQlientSettingsDir(), "UpdateOnPull", true).toBool();

   if (ret.success && updateOnPull)
   {
      QScopedPointer<GitSubmodules> git(new GitSubmodules(mGitBase));
      const auto updateRet = git->submoduleUpdate(QString());

      if (!updateRet)
      {
         return { updateRet,
                  "There was a problem updating the submodules after pull. Please review that you don't have any local "
                  "modifications in the submodules" };
      }
   }

   return ret;
}

bool GitRemote::fetch()
{

   QLog_Debug("Git", QString("Executing fetch with prune"));

   GitQlientSettings settings;
   const auto pruneOnFetch = settings.localValue(mGitBase->getGitQlientSettingsDir(), "PruneOnFetch", true).toBool();

   const auto cmd
       = QString("git fetch --all --tags --force %1").arg(pruneOnFetch ? QString("--prune --prune-tags") : QString());
   const auto ret = mGitBase->run(cmd).success;

   return ret;
}

GitExecResult GitRemote::prune()
{

   QLog_Debug("Git", QString("Executing prune"));

   const auto ret = mGitBase->run("git remote prune origin");

   return ret;
}
