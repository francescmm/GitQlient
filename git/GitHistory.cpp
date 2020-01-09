#include "GitHistory.h"

#include <GitBase.h>

GitHistory::GitHistory(const QSharedPointer<GitBase> &gitBase)
   : mGitBase(gitBase)
{
}

GitExecResult GitHistory::blame(const QString &file, const QString &commitFrom)
{
   return mGitBase->run(QString("git annotate %1 %2").arg(file, commitFrom));
}

GitExecResult GitHistory::history(const QString &file)
{
   return mGitBase->run(QString("git log --follow --pretty=%H %1").arg(file));
}

GitExecResult GitHistory::getCommitDiff(const QString &sha, const QString &diffToSha)
{
   if (!sha.isEmpty())
   {
      QString runCmd = QString("git diff-tree --no-color -r --patch-with-stat -m");

      if (sha != CommitInfo::ZERO_SHA)
      {
         runCmd += " -C ";

         if (diffToSha.isEmpty())
            runCmd += " --root ";

         runCmd.append(QString("%1 %2").arg(diffToSha, sha)); // diffToSha could be empty
      }
      else
         runCmd += " HEAD ";

      return mGitBase->run(runCmd);
   }
   return qMakePair(false, QString());
}

QString GitHistory::getFileDiff(const QString &currentSha, const QString &previousSha, const QString &file)
{
   QByteArray output;
   const auto ret = mGitBase->run(QString("git diff -U15000 %1 %2 %3").arg(previousSha, currentSha, file));

   if (ret.first)
      return ret.second;

   return QString();
}
