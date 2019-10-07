#pragma once

/*
        Author: Marco Costalba (C) 2005-2007

        Copyright: See COPYING file that comes with this distribution

*/

#include <QProcess>

class Git;

// custom process used to run shell commands in parallel

class MyProcess : public QProcess
{
   Q_OBJECT
public:
   MyProcess(QObject *go, const QString &wd, bool reportErrors);
   bool runSync(const QString &runCmd, QByteArray *runOutput, QObject *rcv, const QString &buf);
   bool runAsync(const QString &rc, QObject *rcv, const QString &buf);
   static const QStringList splitArgList(const QString &cmd);
   const QString &getErrorOutput() const { return accError; }

signals:
   void procDataReady(const QByteArray &);
   void eof();

public slots:
   void on_cancel();

private slots:
   void on_readyReadStandardOutput();
   void on_readyReadStandardError();
   void on_finished(int, QProcess::ExitStatus);

private:
   void setupSignals();
   bool launchMe(const QString &runCmd, const QString &buf);
   void sendErrorMsg(bool notStarted = false);
   static void restoreSpaces(QString &newCmd, const QChar &sepChar);

   QObject *guiObject;
   QString runCmd;
   QByteArray *runOutput;
   QString workDir;
   QObject *receiver;
   QStringList arguments;
   QString accError;
   bool errorReportingEnabled;
   bool canceling;
   bool busy;
   bool async;
   bool isErrorExit;
};
