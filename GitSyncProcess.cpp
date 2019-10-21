#include "GitSyncProcess.h"

#include <git.h>

#include <QTemporaryFile>
#include <QTextStream>

GitSyncProcess::GitSyncProcess(const QString &workingDir)
   : AGitProcess(workingDir)
{
}

bool GitSyncProcess::run(const QString &command, QString &output)
{
   mRunOutput = &output;
   mRunOutput->clear();

   const auto processStarted = execute(command);

   if (processStarted)
      waitForFinished(10000);

   close();

   return !mErrorExit;
}
