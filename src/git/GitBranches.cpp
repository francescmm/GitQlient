#include "GitBranches.h"

#include <GitBase.h>
#include <GitConfig.h>
#include <QLogger.h>

using namespace QLogger;

GitBranches::GitBranches(const QSharedPointer<GitBase> &gitBase)
   : mGitBase(gitBase)
{
}

GitExecResult GitBranches::getBranches()
{
   QLog_Debug("Git", "Executing getBranches");

   return mGitBase->run(QString("git branch -a"));
}

bool GitBranches::getDistanceBetweenBranchesAsync(bool toMaster, const QString &right)
{
   QLog_Debug("Git",
              QString("Executing getDistanceBetweenBranches: {origin/%1} and {%2}")
                  .arg(toMaster ? QString("master") : QString("%1"))
                  .arg(right));

   QScopedPointer<GitConfig> gitConfig(new GitConfig(mGitBase));

   const auto ret = gitConfig->getRemoteForBranch(toMaster ? QString("master") : right);

   const QString firstArg = toMaster ? QString("--left-right") : QString();
   const QString gitCmd = QString("git rev-list %1 --count %2/%3...%4")
                              .arg(firstArg)
                              .arg(ret.output.toString())
                              .arg(toMaster ? QString("master") : right)
                              .arg(right);

   connect(mGitBase.get(), &GitBase::signalResultReady, this, &GitBranches::signalDistanceBetweenBranches);

   return mGitBase->runAsync(gitCmd);
}

GitExecResult GitBranches::createBranchFromAnotherBranch(const QString &oldName, const QString &newName)
{
   QLog_Debug("Git", QString("Executing createBranchFromAnotherBranch: {%1} and {%2}").arg(oldName, newName));

   return mGitBase->run(QString("git branch %1 %2").arg(newName, oldName));
}

GitExecResult GitBranches::createBranchAtCommit(const QString &commitSha, const QString &branchName)
{
   QLog_Debug("Git", QString("Executing createBranchAtCommit: {%1} at {%2}").arg(branchName, commitSha));

   return mGitBase->run(QString("git branch %1 %2").arg(branchName, commitSha));
}

GitExecResult GitBranches::checkoutRemoteBranch(const QString &branchName)
{
   QLog_Debug("Git", QString("Executing checkoutRemoteBranch: {%1}").arg(branchName));

   const auto ret = mGitBase->run(QString("git checkout %1").arg(branchName));

   if (ret.success)
      mGitBase->updateCurrentBranch();

   return ret;
}

GitExecResult GitBranches::checkoutNewLocalBranch(const QString &branchName)
{
   QLog_Debug("Git", QString("Executing checkoutNewLocalBranch: {%1}").arg(branchName));

   const auto ret = mGitBase->run(QString("git checkout -b %1").arg(branchName));

   if (ret.success)
      mGitBase->updateCurrentBranch();

   return ret;
}

GitExecResult GitBranches::renameBranch(const QString &oldName, const QString &newName)
{
   QLog_Debug("Git", QString("Executing renameBranch: {%1} at {%2}").arg(oldName, newName));

   const auto ret = mGitBase->run(QString("git branch -m %1 %2").arg(oldName, newName));

   if (ret.success)
      mGitBase->updateCurrentBranch();

   return ret;
}

GitExecResult GitBranches::removeLocalBranch(const QString &branchName)
{
   QLog_Debug("Git", QString("Executing removeLocalBranch: {%1}").arg(branchName));

   return mGitBase->run(QString("git branch -D %1").arg(branchName));
}

GitExecResult GitBranches::removeRemoteBranch(const QString &branchName)
{
   QLog_Debug("Git", QString("Executing removeRemoteBranch: {%1}").arg(branchName));

   return mGitBase->run(QString("git push --delete origin %1").arg(branchName));
}

GitExecResult GitBranches::getBranchesOfCommit(const QString &sha)
{
   QLog_Debug("Git", QString("Executing removeBranchesOfCommit: {%1}").arg(sha));

   return mGitBase->run(QString("git branch --contains %1 --all").arg(sha));
}

GitExecResult GitBranches::getLastCommitOfBranch(const QString &branch)
{
   QLog_Debug("Git", QString("Executing getLastCommitOfBranch: {%1}").arg(branch));

   auto ret = mGitBase->run(QString("git rev-parse %1").arg(branch));

   if (ret.success)
      ret.output = ret.output.toString().trimmed();

   return ret;
}

GitExecResult GitBranches::pushUpstream(const QString &branchName)
{
   QLog_Debug("Git", QString("Executing pushUpstream: {%1}").arg(branchName));

   return mGitBase->run(QString("git push --set-upstream origin %1").arg(branchName));
}
