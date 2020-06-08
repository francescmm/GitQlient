#include "AGitProcess.h"

#include <QTemporaryFile>
#include <QTextStream>

#include <QLogger.h>
#include <QBenchmark.h>

using namespace QLogger;
using namespace QBenchmark;

namespace
{
void restoreSpaces(QString &newCmd, const QChar &sepChar)
{
   QChar quoteChar;
   auto replace = false;
   const auto newCommandLength = newCmd.length();

   for (int i = 0; i < newCommandLength; ++i)
   {
      const auto c = newCmd[i];

      if (!replace && (c == "$"[0] || c == '\"' || c == '\'') && (newCmd.count(c) % 2 == 0))
      {
         replace = true;
         quoteChar = c;
         continue;
      }

      if (replace && (c == quoteChar))
      {
         replace = false;
         continue;
      }

      if (replace && c == sepChar)
         newCmd[i] = QChar(' ');
   }
}

QStringList splitArgList(const QString &cmd)
{
   // return argument list handling quotes and double quotes
   // substring, as example from:
   // cmd some_arg "some thing" v='some value'
   // to (comma separated fields)
   // sl = <cmd,some_arg,some thing,v='some value'>

   // early exit the common case
   if (!(cmd.contains("$") || cmd.contains("\"") || cmd.contains("\'")))
      return cmd.split(' ', QString::SkipEmptyParts);

   // we have some work to do...
   // first find a possible separator
   const QString sepList("#%&!?"); // separator candidates
   int i = 0;
   while (cmd.contains(sepList[i]) && i < sepList.length())
      i++;

   if (i == sepList.length())
      return QStringList();

   const QChar &sepChar(sepList[i]);

   // remove all spaces
   QString newCmd(cmd);
   newCmd.replace(QChar(' '), sepChar);

   // re-add spaces in quoted sections
   restoreSpaces(newCmd, sepChar);

   // "$" is used internally to delimit arguments
   // with quoted text wholly inside as
   // arg1 = <[patch] cool patch on "cool feature">
   // and should be removed before to feed QProcess
   newCmd.remove("$");

   // QProcess::setArguments doesn't want quote
   // delimited arguments, so remove trailing quotes
   auto sl = QStringList(newCmd.split(sepChar, QString::SkipEmptyParts));
   QStringList::iterator it(sl.begin());

   for (; it != sl.end(); ++it)
   {
      if (it->isEmpty())
         continue;

      if (((*it).at(0) == "\"" && (*it).right(1) == "\"") || ((*it).at(0) == "\'" && (*it).right(1) == "\'"))
         *it = (*it).mid(1, (*it).length() - 2);
   }
   return sl;
}
}

AGitProcess::AGitProcess(const QString &workingDir)
   : mWorkingDirectory(workingDir)
{
   setWorkingDirectory(mWorkingDirectory);

   connect(this, &AGitProcess::readyReadStandardOutput, this, &AGitProcess::onReadyStandardOutput,
           Qt::DirectConnection);
   connect(this, static_cast<void (AGitProcess::*)(int, QProcess::ExitStatus)>(&AGitProcess::finished), this,
           &AGitProcess::onFinished, Qt::DirectConnection);
}

void AGitProcess::onCancel()
{
   QBenchmarkStart();

   mCanceling = true;

   waitForFinished();

   QBenchmarkEnd();
}

void AGitProcess::onReadyStandardOutput()
{
   if (!mCanceling)
   {
      const auto standardOutput = readAllStandardOutput();

      mRunOutput.append(QString::fromUtf8(standardOutput));

      emit procDataReady(standardOutput);
   }
}

bool AGitProcess::execute(const QString &command)
{
   mCommand = command;

   auto processStarted = false;
   auto arguments = splitArgList(mCommand);

   if (!arguments.isEmpty())
   {
      QStringList env = QProcess::systemEnvironment();
      env << "GIT_TRACE=0"; // avoid choking on debug traces
      env << "GIT_FLUSH=0"; // skip the fflush() in 'git log'

      setEnvironment(env);
      setProgram(arguments.takeFirst());
      setArguments(arguments);
      start();

      processStarted = waitForStarted();

      if (!processStarted)
         QLog_Warning("Git", QString("Unable to start the process:\n%1\nMore info:\n%2").arg(mCommand, errorString()));
      else
         QLog_Debug("Git", QString("Process started: %1").arg(mCommand));
   }

   return processStarted;
}

void AGitProcess::onFinished(int, QProcess::ExitStatus exitStatus)
{
   QLog_Debug("Git", QString("Process {%1} finished.").arg(mCommand));

   const auto errorOutput = readAllStandardError();

   mErrorOutput = QString::fromUtf8(errorOutput);
   mRealError = exitStatus != QProcess::NormalExit || mCanceling || errorOutput.contains("error")
       || errorOutput.toLower().contains("could not read username");

   if (mRealError)
      mRunOutput = mErrorOutput;
   else
      mRunOutput.append(readAllStandardOutput() + mErrorOutput);
}
