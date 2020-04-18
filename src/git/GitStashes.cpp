#include "GitStashes.h"

#include <GitBase.h>
#include <QLogger.h>

using namespace QLogger;

GitStashes::GitStashes(const QSharedPointer<GitBase> &gitBase)
   : mGitBase(gitBase)
{
}

QVector<QString> GitStashes::getStashes()
{
   QLog_Debug("Git", QString("Executing getStashes"));

   const auto ret = mGitBase->run("git stash list");

   QVector<QString> stashes;

   if (ret.success)
   {
      const auto tagsTmp = ret.output.toString().split("\n");

      for (auto tag : tagsTmp)
         if (tag != "\n" && !tag.isEmpty())
            stashes.append(tag);
   }

   return stashes;
}

GitExecResult GitStashes::pop() const
{
   QLog_Debug("Git", QString("Executing pop"));

   return mGitBase->run("git stash pop");
}

GitExecResult GitStashes::stash()
{
   QLog_Debug("Git", QString("Executing stash"));

   return mGitBase->run("git stash");
}

GitExecResult GitStashes::stashBranch(const QString &stashId, const QString &branchName)
{
   QLog_Debug("Git", QString("Executing stashBranch: {%1} in branch {%2}").arg(stashId, branchName));

   return mGitBase->run(QString("git stash branch %1 %2").arg(branchName, stashId));
}

GitExecResult GitStashes::stashDrop(const QString &stashId)
{
   QLog_Debug("Git", QString("Executing stashDrop: {%1}").arg(stashId));

   return mGitBase->run(QString("git stash drop -q %1").arg(stashId));
}

GitExecResult GitStashes::stashClear()
{
   QLog_Debug("Git", QString("Executing stashClear"));

   return mGitBase->run("git stash clear");
}
