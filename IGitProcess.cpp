#include "IGitProcess.h"

IGitProcess::IGitProcess(const QString &workingDir, bool reportErrorsEnabled)
   : QProcess()
   , mWorkingDirectory(workingDir)
   , mErrorReportingEnabled(reportErrorsEnabled)
{
}

void IGitProcess::onCancel()
{
   mCanceling = true;

#ifdef Q_OS_WIN32
   kill(); // uses TerminateProcess
#else
   terminate(); // uses SIGTERM signal
#endif
   waitForFinished();
}
