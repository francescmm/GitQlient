#include "GitBase.h"

#include <GitSyncProcess.h>
#include <GitAsyncProcess.h>

#include <QLogger.h>
#include <BenchmarkTool.h>

using namespace QLogger;
using namespace Benchmarker;

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
   BenchmarkStart();

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

   BenchmarkEnd();

   return ret;
}

bool GitBase::runAsync(const QString &cmd) const
{
   BenchmarkStart();

   const auto p = new GitAsyncProcess(mWorkingDirectory);
   connect(this, &GitBase::cancelAllProcesses, p, &AGitProcess::onCancel);
   connect(p, &GitAsyncProcess::signalDataReady, this, &GitBase::signalResultReady);

   const auto ret = p->run(cmd);

   BenchmarkEnd();

   return ret.success;
}

void GitBase::updateCurrentBranch()
{
   BenchmarkStart();

   QLog_Trace("Git", "Updating the current branch");

   const auto ret = run("git rev-parse --abbrev-ref HEAD");

   mCurrentBranch = ret.success ? ret.output.toString().trimmed().remove("heads/") : QString();

   BenchmarkEnd();
}

QString GitBase::getCurrentBranch()
{
   QLog_Trace("Git", "Executing getCurrentBranch");

   if (mCurrentBranch.isEmpty())
      updateCurrentBranch();

   return mCurrentBranch;
}

GitExecResult GitBase::getLastCommit() const
{
   BenchmarkStart();

   QLog_Trace("Git", "Executing getLastCommit");

   const auto ret = run("git rev-parse HEAD");

   BenchmarkEnd();

   return ret;
}
