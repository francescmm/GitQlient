/*
        Description: interface to sync and async external program execution

        Author: Marco Costalba (C) 2005-2007

        Copyright: See COPYING file that comes with this distribution

*/
#include <QApplication>
#include <QMessageBox>
#include <QTime>

#include "common.h"
#include "domain.h"
#include "myprocess.h"
#include <git.h>
#include <MainWindow.h>

MyProcess::MyProcess(const QString &wd, bool err)
   : QProcess(Git::getInstance())
{
   workDir = wd;
   errorReportingEnabled = err;
}

bool MyProcess::runAsync(const QString &rc, QObject *rcv, const QString &buf)
{
   async = true;
   runCmd = rc;
   receiver = rcv;
   setupSignals();

   return launchMe(runCmd, buf);
}

bool MyProcess::runSync(const QString &rc, QString &ro)
{
   async = false;
   runCmd = rc;
   runOutput = &ro;

   if (runOutput)
      runOutput->clear();

   setupSignals();

   if (!launchMe(runCmd))
      return false;

   QTime t;
   t.start();

   waitForFinished(10000); // suspend 20ms to let OS reschedule

   return !isErrorExit;
}

void MyProcess::setupSignals()
{
   connect(Git::getInstance(), &Git::cancelAllProcesses, this, &MyProcess::on_cancel);
   connect(this, &MyProcess::readyReadStandardOutput, this, &MyProcess::on_readyReadStandardOutput,
           Qt::DirectConnection);
   connect(this, qOverload<int, QProcess::ExitStatus>(&MyProcess::finished), this, &MyProcess::on_finished,
           Qt::DirectConnection);

   if (receiver && receiver != Git::getInstance())
   {
      connect(this, &MyProcess::readyReadStandardError, this, &MyProcess::on_readyReadStandardError,
              Qt::DirectConnection);
      connect(this, SIGNAL(procDataReady(const QByteArray &)), receiver, SLOT(procReadyRead(const QByteArray &)));
      connect(this, SIGNAL(eof()), receiver, SLOT(procFinished()));
   }

   const auto d = Git::getInstance()->curContext();

   if (d)
      connect(d, &Domain::cancelDomainProcesses, this, &MyProcess::on_cancel);
}

void MyProcess::sendErrorMsg(bool notStarted)
{

   if (!errorReportingEnabled)
      return;

   QByteArray err = readAllStandardError();
   accError += QString::fromUtf8(err);
   QString errorDesc = accError;

   if (notStarted)
      errorDesc = QString::fromLatin1("Unable to start the process!");

   const QString cmd(arguments.join(" ")); // hide any "$" or related stuff

   QString text("An error occurred while executing command:\n\n");
   text.append(cmd + "\n\nGit says: \n\n" + errorDesc);

   if (QGit::kErrorLogBrowser)
   {
      QGit::kErrorLogBrowser->clear();
      QGit::kErrorLogBrowser->setText(text);
   }
}

bool MyProcess::launchMe(const QString &runCmd, const QString &buf)
{

   arguments = splitArgList(runCmd);
   if (arguments.isEmpty())
      return false;

   setWorkingDirectory(workDir);
   if (!QGit::startProcess(this, arguments, buf))
   {
      sendErrorMsg(true);
      return false;
   }
   return true;
}

void MyProcess::on_readyReadStandardOutput()
{

   if (canceling)
      return;

   if (receiver)
      emit procDataReady(readAllStandardOutput());

   else if (runOutput)
      runOutput->append(readAllStandardOutput());
}

void MyProcess::on_readyReadStandardError()
{

   if (canceling)
      return;

   if (receiver)
   {
      QByteArray err = readAllStandardError();
      accError += QString::fromUtf8(err);
      emit procDataReady(err); // redirect to stdout
   }
}

void MyProcess::on_finished(int, QProcess::ExitStatus exitStatus)
{

   // Checking exingStatus is not reliable under Windows where if the
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
   accError += QString::fromUtf8(err);

   isErrorExit = exitStatus != QProcess::NormalExit || canceling || err.contains("error");

   if (!isErrorExit && runOutput)
   {
      emit procDataReady(okOutput + err);

      runOutput->append(okOutput);
      runOutput->append(err);
   }

   if (!canceling)
   { // no more noise after cancel

      if (receiver)
         emit eof();

      if (isErrorExit)
         sendErrorMsg(false);
   }

   if (async)
      deleteLater();
}

void MyProcess::on_cancel()
{

   canceling = true;

#ifdef Q_OS_WIN32
   kill(); // uses TerminateProcess
#else
   terminate(); // uses SIGTERM signal
#endif
   waitForFinished();
}

const QStringList MyProcess::splitArgList(const QString &cmd)
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

void MyProcess::restoreSpaces(QString &newCmd, const QChar &sepChar)
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
