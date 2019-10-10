#include "GitAsyncProcess.h"
#include "git.h"

#include <QTemporaryFile>
#include <QTextStream>

namespace
{
void restoreSpaces(QString &newCmd, const QChar &sepChar)
{
   // restore spaces inside quoted text, supports nested quote types

   QChar quoteChar;
   bool replace = false;
   for (int i = 0; i < newCmd.length(); i++)
   {

      const QChar &c = newCmd[i];

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

const QStringList splitArgList(const QString &cmd)
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
   QStringList sl(newCmd.split(sepChar, QString::SkipEmptyParts));
   QStringList::iterator it(sl.begin());
   for (; it != sl.end(); ++it)
   {
      if (((*it).left(1) == "\"" && (*it).right(1) == "\"") || ((*it).left(1) == "\'" && (*it).right(1) == "\'"))
         *it = (*it).mid(1, (*it).length() - 2);
   }
   return sl;
}
}

GitAsyncProcess::GitAsyncProcess(const QString &workingDir, bool reportErrorsEnabled, QObject *receiver)
   : IGitProcess(workingDir, reportErrorsEnabled)
   , mReceiver(receiver)
{
}

bool GitAsyncProcess::run(const QString &command, QString &buffer)
{
   connect(Git::getInstance(), &Git::cancelAllProcesses, this, &IGitProcess::onCancel);
   connect(this, &GitAsyncProcess::readyReadStandardOutput, this, &GitAsyncProcess::onReadyStandardOutput,
           Qt::DirectConnection);
   connect(this, qOverload<int, QProcess::ExitStatus>(&IGitProcess::finished), this, &GitAsyncProcess::onFinished,
           Qt::DirectConnection);

   if (mReceiver && mReceiver != Git::getInstance())
   {
      connect(this, &IGitProcess::readyReadStandardError, this, &GitAsyncProcess::onReadyStandardError,
              Qt::DirectConnection);
      connect(this, SIGNAL(procDataReady(const QByteArray &)), mReceiver, SLOT(procReadyRead(const QByteArray &)));
      connect(this, SIGNAL(eof()), mReceiver, SLOT(procFinished()));
   }

   mArguments = splitArgList(command);
   if (mArguments.isEmpty())
      return false;

   setWorkingDirectory(mWorkingDirectory);
   if (!startProcess(this, mArguments, buffer))
   {
      if (QGit::kErrorLogBrowser)
      {
         QGit::kErrorLogBrowser->clear();
         QGit::kErrorLogBrowser->setText(
             QString("Unable to start the process:\n\n%1\n\nGit says: \n\n").arg(mArguments.join(" ")));
      }

      return false;
   }
   return true;
}

void GitAsyncProcess::onReadyStandardOutput()
{
   if (!mCanceling)
   {
      if (mReceiver)
         emit procDataReady(readAllStandardOutput());
      else if (mRunOutput)
         mRunOutput->append(readAllStandardOutput());
   }
}

void GitAsyncProcess::onReadyStandardError()
{
   if (mCanceling && mReceiver)
   {
      QByteArray err = readAllStandardError();
      mErrorOutput += QString::fromUtf8(err);
      emit procDataReady(err); // redirect to stdout
   }
}

bool GitAsyncProcess::startProcess(QProcess *proc, QStringList args, const QString &buf)
{
   if (!proc || args.isEmpty())
      return false;

   QStringList arguments(args);
   QString prog(arguments.takeFirst());

   if (!buf.isEmpty())
   {
      /*
         On Windows buffer size of QProcess's standard input
         pipe is quite limited and a crash can occur in case
         a big chunk of data is written to process stdin.
         As a workaround we use a temporary file to store data.
         Process stdin will be redirected to this file
      */
      QTemporaryFile *bufFile = new QTemporaryFile(proc);
      bufFile->open();
      QTextStream stream(bufFile);
      stream << buf;
      proc->setStandardInputFile(bufFile->fileName());
      bufFile->close();
   }

   QStringList env = QProcess::systemEnvironment();
   env << "GIT_TRACE=0"; // avoid choking on debug traces
   env << "GIT_FLUSH=0"; // skip the fflush() in 'git log'
   proc->setEnvironment(env);

   proc->start(prog, arguments); // TODO test QIODevice::Unbuffered
   return proc->waitForStarted();
}

void GitAsyncProcess::onFinished(int, QProcess::ExitStatus exitStatus)
{ // Checking exingStatus is not reliable under Windows where if the
   // process was terminated with TerminateProcess() from another
   // application its value is still NormalExit
   //
   // Checking exit code for a failing command is unreliable too, as
   // exmple 'git status' returns 1 also without errors.
   //
   // On Windows exit code seems reliable in case of a command wrapped
   // in Window shell interpreter.
   //
   // So to detect a failing command we check also if stderr is not empty.
   QByteArray err = readAllStandardError();
   auto okOutput = readAllStandardOutput();
   mErrorOutput += QString::fromUtf8(err);

   mErrorExit = exitStatus != QProcess::NormalExit || mCanceling || err.contains("error");

   if (!mErrorExit && mRunOutput)
   {
      emit procDataReady(okOutput + err);

      mRunOutput->append(okOutput);
      mRunOutput->append(err);
   }

   if (!mCanceling)
   { // no more noise after cancel

      if (mReceiver)
         emit eof();

      if (mErrorExit && mErrorReportingEnabled)
      {
         QByteArray err = readAllStandardError();
         mErrorOutput += QString::fromUtf8(err);
         QString errorDesc = mErrorOutput;

         const QString cmd(mArguments.join(" ")); // hide any "$" or related stuff

         QString text("An error occurred while executing command:\n\n");
         text.append(cmd + "\n\nGit says: \n\n" + errorDesc);

         if (QGit::kErrorLogBrowser)
         {
            QGit::kErrorLogBrowser->clear();
            QGit::kErrorLogBrowser->setText(text);
         }
      }
   }

   deleteLater();
}
