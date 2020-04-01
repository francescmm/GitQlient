#include "GitAsyncProcess.h"

#include <QTemporaryFile>
GitAsyncProcess::GitAsyncProcess(const QString &workingDir)
   : AGitProcess(workingDir)
{
}

GitExecResult GitAsyncProcess::run(const QString &command)
{
   return { execute(command), "" };
}

void GitAsyncProcess::onFinished(int code, QProcess::ExitStatus exitStatus)
{
   AGitProcess::onFinished(code, exitStatus);

   if (!mCanceling)
      emit signalDataReady({ mRealError, mRunOutput });

   deleteLater();
}
