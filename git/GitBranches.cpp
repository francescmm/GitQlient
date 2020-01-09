#include "GitBranches.h"

#include <GitBase.h>

GitBranches::GitBranches(const QSharedPointer<GitBase> &gitBase)
   : mGitBase(gitBase)
{
}

GitExecResult GitBranches::getBranches()
{
   return mGitBase->run(QString("git branch -a"));
}

GitExecResult GitBranches::getDistanceBetweenBranches(bool toMaster, const QString &right)
{
   const QString firstArg = toMaster ? QString::fromUtf8("--left-right") : QString::fromUtf8("");
   const QString gitCmd = QString("git rev-list %1 --count %2...%3")
                              .arg(firstArg)
                              .arg(toMaster ? QString::fromUtf8("origin/master") : QString::fromUtf8("origin/%3"))
                              .arg(right);

   return mGitBase->run(gitCmd);
}

GitExecResult GitBranches::createBranchFromAnotherBranch(const QString &oldName, const QString &newName)
{
   return mGitBase->run(QString("git branch %1 %2").arg(newName, oldName));
}

GitExecResult GitBranches::createBranchAtCommit(const QString &commitSha, const QString &branchName)
{
   return mGitBase->run(QString("git branch %1 %2").arg(branchName, commitSha));
}

GitExecResult GitBranches::checkoutRemoteBranch(const QString &branchName)
{
   return mGitBase->run(QString("git checkout -q %1").arg(branchName));
}

GitExecResult GitBranches::checkoutNewLocalBranch(const QString &branchName)
{
   return mGitBase->run(QString("git checkout -b %1").arg(branchName));
}

GitExecResult GitBranches::renameBranch(const QString &oldName, const QString &newName)
{
   return mGitBase->run(QString("git branch -m %1 %2").arg(oldName, newName));
}

GitExecResult GitBranches::removeLocalBranch(const QString &branchName)
{
   return mGitBase->run(QString("git branch -D %1").arg(branchName));
}

GitExecResult GitBranches::removeRemoteBranch(const QString &branchName)
{
   return mGitBase->run(QString("git push --delete origin %1").arg(branchName));
}

GitExecResult GitBranches::getBranchesOfCommit(const QString &sha)
{
   return mGitBase->run(QString("git branch --contains %1 --all").arg(sha));
}

GitExecResult GitBranches::getLastCommitOfBranch(const QString &branch)
{
   auto ret = mGitBase->run(QString("git rev-parse %1").arg(branch));

   if (ret.first)
      ret.second.remove(ret.second.count() - 1, ret.second.count());

   return ret;
}

GitExecResult GitBranches::pushUpstream(const QString &branchName)
{
   return mGitBase->run(QString("git push --set-upstream origin %1").arg(branchName));
}
