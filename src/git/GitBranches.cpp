#include "GitBranches.h"

#include <GitBase.h>
#include <GitConfig.h>

#include <QLogger.h>
#include <BenchmarkTool.h>

using namespace QLogger;
using namespace Benchmarker;

GitBranches::GitBranches(const QSharedPointer<GitBase> &gitBase)
   : mGitBase(gitBase)
{
}

GitExecResult GitBranches::getBranches()
{
   BenchmarkStart();

   QLog_Debug("Git", "Executing getBranches");

   const auto ret = mGitBase->run(QString("git branch -a"));

   BenchmarkEnd();

   return ret;
}

GitExecResult GitBranches::getDistanceBetweenBranches(bool toMaster, const QString &right)
{
   BenchmarkStart();

   QLog_Debug("Git",
              QString("Executing getDistanceBetweenBranches: {origin/%1} and {%2}")
                  .arg(toMaster ? QString("master") : right, right));

   QScopedPointer<GitConfig> gitConfig(new GitConfig(mGitBase));

   const auto ret = gitConfig->getRemoteForBranch(toMaster ? QString("master") : right);
   GitExecResult result;

   if (!toMaster && right == "master")
      result = GitExecResult { false, "Same branch" };
   else
   {
      const auto remote = ret.success ? ret.output.toString().append("/") : QString();
      const auto gitBase = new GitBase(mGitBase->getWorkingDir());
      const auto gitCmd = QString("git rev-list --left-right --count %1%2...%3")
                              .arg(remote, toMaster ? QString("master") : right, right);

      result = gitBase->run(gitCmd);
   }

   BenchmarkEnd();

   return result;
}

GitExecResult GitBranches::createBranchFromAnotherBranch(const QString &oldName, const QString &newName)
{
   BenchmarkStart();
   QLog_Debug("Git", QString("Executing createBranchFromAnotherBranch: {%1} and {%2}").arg(oldName, newName));

   const auto ret = mGitBase->run(QString("git branch %1 %2").arg(newName, oldName));

   BenchmarkEnd();

   return ret;
}

GitExecResult GitBranches::createBranchAtCommit(const QString &commitSha, const QString &branchName)
{
   BenchmarkStart();

   QLog_Debug("Git", QString("Executing createBranchAtCommit: {%1} at {%2}").arg(branchName, commitSha));

   const auto ret = mGitBase->run(QString("git branch %1 %2").arg(branchName, commitSha));

   BenchmarkEnd();

   return ret;
}

GitExecResult GitBranches::checkoutLocalBranch(const QString &branchName)
{
   BenchmarkStart();

   QLog_Debug("Git", QString("Executing checkoutRemoteBranch: {%1}").arg(branchName));

   const auto ret = mGitBase->run(QString("git checkout %1").arg(branchName));

   if (ret.success)
      mGitBase->updateCurrentBranch();

   BenchmarkEnd();

   return ret;
}

GitExecResult GitBranches::checkoutRemoteBranch(const QString &branchName)
{
   BenchmarkStart();

   QLog_Debug("Git", QString("Executing checkoutRemoteBranch: {%1}").arg(branchName));

   auto localBranch = branchName;
   if (localBranch.startsWith("origin/"))
      localBranch.remove("origin/");

   auto ret = mGitBase->run(QString("git checkout -b %1 %2").arg(localBranch, branchName));
   const auto output = ret.output.toString();

   if (ret.success && !output.contains("fatal:"))
      mGitBase->updateCurrentBranch();
   else if (output.contains("already exists"))
   {
      QRegExp rx("\'\\w+\'");
      rx.indexIn(ret.output.toString());
      auto value = rx.capturedTexts().constFirst();
      value.remove("'");

      if (!value.isEmpty())
         ret = checkoutLocalBranch(value);
      else
         ret.success = false;
   }

   BenchmarkEnd();

   return ret;
}

GitExecResult GitBranches::checkoutNewLocalBranch(const QString &branchName)
{
   BenchmarkStart();

   QLog_Debug("Git", QString("Executing checkoutNewLocalBranch: {%1}").arg(branchName));

   const auto ret = mGitBase->run(QString("git checkout -b %1").arg(branchName));

   if (ret.success)
      mGitBase->updateCurrentBranch();

   BenchmarkEnd();

   return ret;
}

GitExecResult GitBranches::renameBranch(const QString &oldName, const QString &newName)
{
   BenchmarkStart();

   QLog_Debug("Git", QString("Executing renameBranch: {%1} at {%2}").arg(oldName, newName));

   const auto ret = mGitBase->run(QString("git branch -m %1 %2").arg(oldName, newName));

   if (ret.success)
      mGitBase->updateCurrentBranch();

   BenchmarkEnd();

   return ret;
}

GitExecResult GitBranches::removeLocalBranch(const QString &branchName)
{
   BenchmarkStart();

   QLog_Debug("Git", QString("Executing removeLocalBranch: {%1}").arg(branchName));

   const auto ret = mGitBase->run(QString("git branch -D %1").arg(branchName));

   BenchmarkEnd();

   return ret;
}

GitExecResult GitBranches::removeRemoteBranch(const QString &branchName)
{
   BenchmarkStart();

   auto branch = branchName;
   branch = branch.mid(branch.indexOf('/') + 1);

   QLog_Debug("Git", QString("Executing removeRemoteBranch: {%1}").arg(branch));

   QScopedPointer<GitConfig> gitConfig(new GitConfig(mGitBase));

   auto ret = gitConfig->getRemoteForBranch(branch);

   ret = mGitBase->run(
       QString("git push --delete %2 %1").arg(branch, ret.success ? ret.output.toString() : QString("origin")));

   BenchmarkEnd();

   return ret;
}

GitExecResult GitBranches::getLastCommitOfBranch(const QString &branch)
{
   BenchmarkStart();

   QLog_Debug("Git", QString("Executing getLastCommitOfBranch: {%1}").arg(branch));

   auto ret = mGitBase->run(QString("git rev-parse %1").arg(branch));

   if (ret.success)
      ret.output = ret.output.toString().trimmed();

   BenchmarkEnd();

   return ret;
}

GitExecResult GitBranches::pushUpstream(const QString &branchName)
{
   BenchmarkStart();

   QLog_Debug("Git", QString("Executing pushUpstream: {%1}").arg(branchName));

   const auto ret = mGitBase->run(QString("git push --set-upstream origin %1").arg(branchName));

   BenchmarkEnd();

   return ret;
}

QMap<QString, QStringList> GitBranches::getTrackingBranches() const
{
   BenchmarkStart();

   QLog_Debug("Git", QString("Executing getTrackingBranches"));

   const auto ret = mGitBase->run(QString("git branch -vv"));
   QMap<QString, QStringList> trackings;

   if (ret.success)
   {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
      const auto output = ret.output.toString().split("\n", Qt::SkipEmptyParts);
#else
      const auto output = ret.output.toString().split("\n", QString::SkipEmptyParts);
#endif
      for (auto line : output)
      {
         if (line.startsWith("*"))
            line.remove("*");

         if (line.contains("["))
         {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
            const auto fields = line.split(' ', Qt::SkipEmptyParts);
#else
            const auto fields = line.split(' ', QString::SkipEmptyParts);
#endif
            auto remote = fields.at(2);
            remote.remove('[').remove(']');
            trackings[remote].append(fields.at(0));
         }
      }
   }

   BenchmarkEnd();

   return trackings;
}
