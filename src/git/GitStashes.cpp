#include "GitStashes.h"

#include <GitBase.h>

#include <QLogger.h>
#include <BenchmarkTool.h>

using namespace QLogger;
using namespace GitQlientTools;

GitStashes::GitStashes(const QSharedPointer<GitBase> &gitBase)
   : mGitBase(gitBase)
{
}

QVector<QString> GitStashes::getStashes()
{
   BenchmarkStart();

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

   BenchmarkEnd();

   return stashes;
}

GitExecResult GitStashes::pop() const
{
   BenchmarkStart();

   QLog_Debug("Git", QString("Executing pop"));

   const auto ret = mGitBase->run("git stash pop");

   BenchmarkEnd();

   return ret;
}

GitExecResult GitStashes::stash()
{
   BenchmarkStart();

   QLog_Debug("Git", QString("Executing stash"));

   const auto ret = mGitBase->run("git stash");

   BenchmarkEnd();

   return ret;
}

GitExecResult GitStashes::stashBranch(const QString &stashId, const QString &branchName)
{
   BenchmarkStart();

   QLog_Debug("Git", QString("Executing stashBranch: {%1} in branch {%2}").arg(stashId, branchName));

   const auto ret = mGitBase->run(QString("git stash branch %1 %2").arg(branchName, stashId));

   BenchmarkEnd();

   return ret;
}

GitExecResult GitStashes::stashDrop(const QString &stashId)
{
   BenchmarkStart();

   QLog_Debug("Git", QString("Executing stashDrop: {%1}").arg(stashId));

   const auto ret = mGitBase->run(QString("git stash drop -q %1").arg(stashId));

   BenchmarkEnd();

   return ret;
}

GitExecResult GitStashes::stashClear()
{
   BenchmarkStart();

   QLog_Debug("Git", QString("Executing stashClear"));

   const auto ret = mGitBase->run("git stash clear");

   BenchmarkEnd();

   return ret;
}
