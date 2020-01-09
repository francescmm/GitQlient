#include "GitLocal.h"

#include <GitBase.h>

GitLocal::GitLocal(const QSharedPointer<GitBase> &gitBase)
   : mGitBase(gitBase)
{
}

GitExecResult GitLocal::cherryPickCommit(const QString &sha)
{
   return mGitBase->run(QString("git cherry-pick %1").arg(sha));
}

GitExecResult GitLocal::checkoutCommit(const QString &sha)
{
   return mGitBase->run(QString("git checkout %1").arg(sha));
}

GitExecResult GitLocal::markFileAsResolved(const QString &fileName)
{
   const auto ret = mGitBase->run(QString("git add %1").arg(fileName));

   /*
   if (ret.first)
      emit signalWipUpdated();
*/

   return ret;
}

bool GitLocal::checkoutFile(const QString &fileName)
{
   if (fileName.isEmpty())
      return false;

   return mGitBase->run(QString("git checkout %1").arg(fileName)).first;
}

GitExecResult GitLocal::resetFile(const QString &fileName)
{
   return mGitBase->run(QString("git reset %1").arg(fileName));
}

bool GitLocal::resetCommit(const QString &sha, CommitResetType type)
{
   QString typeStr;

   switch (type)
   {
      case CommitResetType::SOFT:
         typeStr = "soft";
         break;
      case CommitResetType::MIXED:
         typeStr = "mixed";
         break;
      case CommitResetType::HARD:
         typeStr = "hard";
         break;
   }

   return mGitBase->run(QString("git reset --%1 %2").arg(typeStr, sha)).first;
}
