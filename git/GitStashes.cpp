#include "GitStashes.h"

#include <GitBase.h>

GitStashes::GitStashes(const QSharedPointer<GitBase> &gitBase)
   : mGitBase(gitBase)
{
}

QVector<QString> GitStashes::getStashes()
{
   const auto ret = mGitBase->run("git stash list");

   QVector<QString> stashes;

   if (ret.first)
   {
      const auto tagsTmp = ret.second.split("\n");

      for (auto tag : tagsTmp)
         if (tag != "\n" && !tag.isEmpty())
            stashes.append(tag);
   }

   return stashes;
}

GitExecResult GitStashes::pop() const
{
   return mGitBase->run("git stash pop");
}

GitExecResult GitStashes::stash()
{
   return mGitBase->run("git stash");
}

GitExecResult GitStashes::stashBranch(const QString &stashId, const QString &branchName)
{
   return mGitBase->run(QString("git stash branch %1 %2").arg(branchName, stashId));
}

GitExecResult GitStashes::stashDrop(const QString &stashId)
{
   return mGitBase->run(QString("git stash drop -q %1").arg(stashId));
}

GitExecResult GitStashes::stashClear()
{
   return mGitBase->run("git stash clear");
}
