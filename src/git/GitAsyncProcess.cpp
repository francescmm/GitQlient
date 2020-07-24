#include "GitAsyncProcess.h"

#include <BenchmarkTool.h>

using namespace Benchmarker;

GitAsyncProcess::GitAsyncProcess(const QString &workingDir)
   : AGitProcess(workingDir)
{
}

GitExecResult GitAsyncProcess::run(const QString &command)
{
   BenchmarkStart();

   const auto ret = execute(command);

   BenchmarkEnd();

   return { ret, "" };
}

void GitAsyncProcess::onFinished(int code, QProcess::ExitStatus exitStatus)
{
   BenchmarkStart();

   AGitProcess::onFinished(code, exitStatus);

   if (!mCanceling)
      emit signalDataReady({ mRealError, mRunOutput });

   deleteLater();

   BenchmarkEnd();
}
