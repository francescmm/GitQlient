#include "GitBase.h"

#include <GitSyncProcess.h>
#include <GitAsyncProcess.h>

#include <QLogger.h>
#include <QBenchmark.h>

using namespace QLogger;
using namespace QBenchmark;

#include <QDir>

GitBase::GitBase(const QString &workingDirectory, QObject *parent)
   : QObject(parent)
   , mWorkingDirectory(workingDirectory)
{
}

QString GitBase::getWorkingDir() const
{
   return mWorkingDirectory;
}

void GitBase::setWorkingDir(const QString &workingDir)
{
   mWorkingDirectory = workingDir;
}

GitExecResult GitBase::run(const QString &cmd) const
{
   QBenchmarkStart();

   GitSyncProcess p(mWorkingDirectory);
   connect(this, &GitBase::cancelAllProcesses, &p, &AGitProcess::onCancel);

   const auto ret = p.run(cmd);
   const auto runOutput = ret.output.toString();

   if (ret.success)
   {

      if (runOutput.contains("fatal:"))
         QLog_Info("Git", QString("Git command {%1} reported issues:\n%2").arg(cmd, runOutput));
      else
         QLog_Trace("Git", QString("Git command {%1} executed successfully.").arg(cmd));
   }
   else
      QLog_Warning("Git", QString("Git command {%1} has errors:\n%2").arg(cmd, runOutput));

   QBenchmarkEnd();

   return ret;
}

bool GitBase::runAsync(const QString &cmd) const
{
   QBenchmarkStart();

   const auto p = new GitAsyncProcess(mWorkingDirectory);
   connect(this, &GitBase::cancelAllProcesses, p, &AGitProcess::onCancel);
   connect(p, &GitAsyncProcess::signalDataReady, this, &GitBase::signalResultReady);

   const auto ret = p->run(cmd);

   QBenchmarkEnd();

   return ret.success;
}

void GitBase::updateCurrentBranch()
{
   QBenchmarkStart();

   QLog_Trace("Git", "Updating the current branch");

   const auto ret = run("git rev-parse --abbrev-ref HEAD");

   mCurrentBranch = ret.success ? ret.output.toString().trimmed().remove("heads/") : QString();

   QBenchmarkEnd();
}

QString GitBase::getCurrentBranch()
{
   QBenchmarkStart();

   QLog_Trace("Git", "Executing getCurrentBranch");

   if (mCurrentBranch.isEmpty())
      updateCurrentBranch();

   QBenchmarkEnd();

   return mCurrentBranch;
}

GitExecResult GitBase::getLastCommit() const
{
   QBenchmarkStart();

   const auto ret = run("git rev-parse HEAD");

   QBenchmarkEnd();

   return ret;
}
