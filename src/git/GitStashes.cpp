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

      for (const auto &tag : tagsTmp)
         if (tag != "\n" && !tag.isEmpty())
            stashes.append(tag);
   }

   return stashes;
}

GitExecResult GitStashes::pop() const
{

   QLog_Debug("Git", QString("Executing pop"));

   const auto ret = mGitBase->run("git stash pop");

   return ret;
}

GitExecResult GitStashes::stash()
{

   QLog_Debug("Git", QString("Executing stash"));

   const auto ret = mGitBase->run("git stash");

   return ret;
}

GitExecResult GitStashes::stashBranch(const QString &stashId, const QString &branchName)
{

   QLog_Debug("Git", QString("Executing stashBranch: {%1} in branch {%2}").arg(stashId, branchName));

   const auto ret = mGitBase->run(QString("git stash branch %1 %2").arg(branchName, stashId));

   return ret;
}

GitExecResult GitStashes::stashDrop(const QString &stashId)
{

   QLog_Debug("Git", QString("Executing stashDrop: {%1}").arg(stashId));

   const auto ret = mGitBase->run(QString("git stash drop -q %1").arg(stashId));

   return ret;
}

GitExecResult GitStashes::stashClear()
{

   QLog_Debug("Git", QString("Executing stashClear"));

   const auto ret = mGitBase->run("git stash clear");

   return ret;
}
