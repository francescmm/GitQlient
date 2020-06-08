#include "GitStashes.h"

#include <GitBase.h>

#include <QLogger.h>
#include <QBenchmark.h>

using namespace QLogger;
using namespace QBenchmark;

GitStashes::GitStashes(const QSharedPointer<GitBase> &gitBase)
   : mGitBase(gitBase)
{
}

QVector<QString> GitStashes::getStashes()
{
   QBenchmarkStart();

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

   QBenchmarkEnd();

   return stashes;
}

GitExecResult GitStashes::pop() const
{
   QBenchmarkStart();

   QLog_Debug("Git", QString("Executing pop"));

   const auto ret = mGitBase->run("git stash pop");

   QBenchmarkEnd();

   return ret;
}

GitExecResult GitStashes::stash()
{
   QBenchmarkStart();

   QLog_Debug("Git", QString("Executing stash"));

   const auto ret = mGitBase->run("git stash");

   QBenchmarkEnd();

   return ret;
}

GitExecResult GitStashes::stashBranch(const QString &stashId, const QString &branchName)
{
   QBenchmarkStart();

   QLog_Debug("Git", QString("Executing stashBranch: {%1} in branch {%2}").arg(stashId, branchName));

   const auto ret = mGitBase->run(QString("git stash branch %1 %2").arg(branchName, stashId));

   QBenchmarkEnd();

   return ret;
}

GitExecResult GitStashes::stashDrop(const QString &stashId)
{
   QBenchmarkStart();

   QLog_Debug("Git", QString("Executing stashDrop: {%1}").arg(stashId));

   const auto ret = mGitBase->run(QString("git stash drop -q %1").arg(stashId));

   QBenchmarkEnd();

   return ret;
}

GitExecResult GitStashes::stashClear()
{
   QBenchmarkStart();

   QLog_Debug("Git", QString("Executing stashClear"));

   const auto ret = mGitBase->run("git stash clear");

   QBenchmarkEnd();

   return ret;
}
