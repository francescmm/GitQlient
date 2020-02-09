#include "GitCloneProcess.h"

GitCloneProcess::GitCloneProcess(const QString &workingDir)
   : AGitProcess(workingDir)
{
   connect(this, &AGitProcess::readyReadStandardError, this, &GitCloneProcess::onReadyStandardError,
           Qt::DirectConnection);
}

bool GitCloneProcess::run(const QString &command, QString &)
{
   return execute(command);
}

void GitCloneProcess::onReadyStandardError()
{
   if (!mCanceling)
   {
      const auto err = readAllStandardError();
      const auto errStr = QString::fromUtf8(err);
      mErrorOutput += errStr;

      if (!errStr.startsWith("remote: "))
      {
         auto infoList = errStr.split(",", QString::SkipEmptyParts).first().split(":");
         const auto stepDesc = infoList.takeFirst();
         auto value = -1;

         if (!infoList.isEmpty())
            value = infoList.takeFirst().split("% ").first().trimmed().toInt();

         emit signalProgress(stepDesc, value);
      }
   }
}

void GitCloneProcess::onFinished(int code, QProcess::ExitStatus exitStatus)
{
   AGitProcess::onFinished(code, exitStatus);

   if (!mCanceling)
      emit signalProgress("Done!", 100);

   deleteLater();
}
