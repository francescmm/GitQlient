#include "GitAsyncProcess.h"

#include <QBenchmark.h>

using namespace QBenchmark;

GitAsyncProcess::GitAsyncProcess(const QString &workingDir)
   : AGitProcess(workingDir)
{
}

GitExecResult GitAsyncProcess::run(const QString &command)
{
   QBenchmarkStart();

   const auto ret = execute(command);

   QBenchmarkEnd();

   return { ret, "" };
}

void GitAsyncProcess::onFinished(int code, QProcess::ExitStatus exitStatus)
{
   QBenchmarkStart();

   AGitProcess::onFinished(code, exitStatus);

   if (!mCanceling)
      emit signalDataReady({ mRealError, mRunOutput });

   deleteLater();

   QBenchmarkEnd();
}
