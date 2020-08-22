#include "GitHistory.h"

#include <GitBase.h>
#include <GitConfig.h>

#include <QLogger.h>

#include <QStringLiteral>

using namespace QLogger;

GitHistory::GitHistory(const QSharedPointer<GitBase> &gitBase)
   : mGitBase(gitBase)
{
}

GitExecResult GitHistory::blame(const QString &file, const QString &commitFrom)
{
   QLog_Debug("Git", QString("Executing blame: {%1} from {%2}").arg(file, commitFrom));

   const auto ret = mGitBase->run(QString("git annotate %1 %2").arg(file, commitFrom));

   return ret;
}

GitExecResult GitHistory::history(const QString &file)
{
   QLog_Debug("Git", QString("Executing history: {%1}").arg(file));

   auto ret = mGitBase->run(QString("git log --follow --pretty=%H %1").arg(file));

   if (ret.success && ret.output.toString().isEmpty())
      ret.success = false;

   return ret;
}

GitExecResult GitHistory::getBranchesDiff(const QString &base, const QString &head)
{
   QScopedPointer<GitConfig> git(new GitConfig(mGitBase));

   QString fullBase = base;
   auto retBase = git->getRemoteForBranch(base);

   if (retBase.success)
      fullBase.prepend(retBase.output.toString() + QStringLiteral("/"));

   QString fullHead = head;
   auto retHead = git->getRemoteForBranch(head);

   if (retHead.success)
      fullHead.prepend(retHead.output.toString() + QStringLiteral("/"));

   return mGitBase->run(QString("git diff %1...%2").arg(fullBase, fullHead));
}

GitExecResult GitHistory::getCommitDiff(const QString &sha, const QString &diffToSha)
{
   if (!sha.isEmpty())
   {
      QLog_Debug("Git", QString("Executing getCommitDiff: {%1} to {%2}").arg(sha, diffToSha));

      QString runCmd = QString("git diff-tree --no-color -r --patch-with-stat -m");

      if (sha != CommitInfo::ZERO_SHA)
      {
         runCmd += " -C ";

         if (diffToSha.isEmpty())
            runCmd += " --root ";

         runCmd.append(QString("%1 %2").arg(diffToSha, sha)); // diffToSha could be empty
      }
      else
         runCmd = "git diff HEAD ";

      return mGitBase->run(runCmd);
   }
   else
      QLog_Warning("Git", QString("Executing getCommitDiff with empty SHA"));

   return qMakePair(false, QString());
}

QString GitHistory::getFileDiff(const QString &currentSha, const QString &previousSha, const QString &file)
{
   QLog_Debug("Git", QString("Executing getFileDiff: {%1} between {%2} and {%3}").arg(file, currentSha, previousSha));

   const auto ret = mGitBase->run(QString("git diff -w -U15000 %1 %2 %3").arg(previousSha, currentSha, file));

   if (ret.success)
   {

      return ret.output.toString();
   }

   return QString();
}

GitExecResult GitHistory::getDiffFiles(const QString &sha, const QString &diffToSha)
{
   QLog_Debug("Git", QString("Executing getDiffFiles: {%1} to {%2}").arg(sha, diffToSha));

   QString runCmd = QString("git diff-tree -C --no-color -r -m ");

   if (!diffToSha.isEmpty() && sha != CommitInfo::ZERO_SHA)
      runCmd.append(diffToSha + " " + sha);

   return mGitBase->run(runCmd);
}
