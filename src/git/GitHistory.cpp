#include "GitHistory.h"

#include <GitBase.h>

#include <QLogger.h>
#include <QBenchmark.h>

using namespace QLogger;
using namespace QBenchmark;

GitHistory::GitHistory(const QSharedPointer<GitBase> &gitBase)
   : mGitBase(gitBase)
{
}

GitExecResult GitHistory::blame(const QString &file, const QString &commitFrom)
{
   QBenchmarkStart();

   QLog_Debug("Git", QString("Executing blame: {%1} from {%2}").arg(file, commitFrom));

   const auto ret = mGitBase->run(QString("git annotate %1 %2").arg(file, commitFrom));

   QBenchmarkEnd();

   return ret;
}

GitExecResult GitHistory::history(const QString &file)
{
   QBenchmarkStart();

   QLog_Debug("Git", QString("Executing history: {%1}").arg(file));

   auto ret = mGitBase->run(QString("git log --follow --pretty=%H %1").arg(file));

   if (ret.success && ret.output.toString().isEmpty())
      ret.success = false;

   QBenchmarkEnd();

   return ret;
}

GitExecResult GitHistory::getCommitDiff(const QString &sha, const QString &diffToSha)
{
   QBenchmarkStart();

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

      QBenchmarkEnd();

      return mGitBase->run(runCmd);
   }
   else
      QLog_Warning("Git", QString("Executing getCommitDiff with empty SHA"));

   QBenchmarkEnd();

   return qMakePair(false, QString());
}

QString GitHistory::getFileDiff(const QString &currentSha, const QString &previousSha, const QString &file)
{
   QBenchmarkStart();

   QLog_Debug("Git", QString("Executing getFileDiff: {%1} between {%2} and {%3}").arg(file, currentSha, previousSha));

   const auto ret = mGitBase->run(QString("git diff -U15000 %1 %2 %3").arg(previousSha, currentSha, file));

   if (ret.success)
   {
      QBenchmarkEnd();
      return ret.output.toString();
   }

   QBenchmarkEnd();
   return QString();
}

GitExecResult GitHistory::getDiffFiles(const QString &sha, const QString &diffToSha)
{
   QBenchmarkStart();

   QLog_Debug("Git", QString("Executing getDiffFiles: {%1} to {%2}").arg(sha, diffToSha));

   QString runCmd = QString("git diff-tree -C --no-color -r -m ");

   if (!diffToSha.isEmpty() && sha != CommitInfo::ZERO_SHA)
      runCmd.append(diffToSha + " " + sha);

   QBenchmarkEnd();

   return mGitBase->run(runCmd);
}
