#include "GitSyncProcess.h"

#include <git.h>

#include <QTemporaryFile>
#include <QTextStream>

GitSyncProcess::GitSyncProcess(const QString &workingDir, bool reportErrorsEnabled)
   : AGitProcess(workingDir, reportErrorsEnabled)
{
}

bool GitSyncProcess::run(const QString &command, QString &output)
{
   mRunOutput = &output;
   mRunOutput->clear();

   const auto processStarted = execute(command);

   if (processStarted)
      waitForFinished(10000); // suspend 20ms to let OS reschedule

   return !mErrorExit;
}
