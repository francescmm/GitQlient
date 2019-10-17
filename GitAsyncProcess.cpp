#include "GitAsyncProcess.h"

#include "git.h"

GitAsyncProcess::GitAsyncProcess(const QString &workingDir, QObject *receiver)
   : AGitProcess(workingDir)
   , mReceiver(receiver)
{
   if (mReceiver)
   {
      connect(this, &AGitProcess::readyReadStandardError, this, &GitAsyncProcess::onReadyStandardError,
              Qt::DirectConnection);
      connect(this, SIGNAL(procDataReady(const QByteArray &)), mReceiver, SLOT(procReadyRead(const QByteArray &)));
      connect(this, SIGNAL(eof()), mReceiver, SLOT(procFinished()));
   }
}

bool GitAsyncProcess::run(const QString &command, QString &)
{
   return execute(command);
}

void GitAsyncProcess::onReadyStandardError()
{
   if (mCanceling && mReceiver)
   {
      const auto err = readAllStandardError();
      mErrorOutput += QString::fromUtf8(err);

      emit procDataReady(err);
   }
}

void GitAsyncProcess::onFinished(int code, QProcess::ExitStatus exitStatus)
{
   AGitProcess::onFinished(code, exitStatus);

   if (!mCanceling && mReceiver)
      emit eof();

   deleteLater();
}
