#include "GitBase.h"

#include <GitRequestorProcess.h>
#include <GitSyncProcess.h>

#include <QLogger.h>
using namespace QLogger;

#include <QDir>

GitBase::GitBase(const QString &workingDirectory, QObject *parent)
   : QObject(parent)
   , mWorkingDirectory(workingDirectory)
{
}

QPair<bool, QString> GitBase::run(const QString &runCmd) const
{
   QString runOutput;
   GitSyncProcess p(mWorkingDirectory);
   connect(this, &GitBase::cancelAllProcesses, &p, &AGitProcess::onCancel);

   const auto ret = p.run(runCmd, runOutput);

   return qMakePair(ret, runOutput);
}

QString GitBase::getCurrentBranch() const
{
   const auto ret = run("git rev-parse --abbrev-ref HEAD");

   return ret.first ? ret.second.trimmed() : QString();
}
