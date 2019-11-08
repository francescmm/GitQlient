/*
     Author: Marco Costalba (C) 2005-2007

Copyright: See COPYING file that comes with this distribution

             */
#ifndef DATALOADER_H
#define DATALOADER_H

#include <QProcess>
#include <QTime>
#include <QTimer>
#include <QSharedPointer>

class Git;
class QString;
class UnbufferedTemporaryFile;

// data exchange facility with 'git log' could be based on QProcess or on
// a temporary file (default). Uncomment following line to use QProcess
// #define USE_QPROCESS

class DataLoader : public QProcess
{
   Q_OBJECT
public:
   DataLoader(Git *git);
   ~DataLoader();
   bool start(const QStringList &args, const QString &wd, const QString &buf);
   void on_cancel();

signals:
   void newDataReady();
   void loaded();

private slots:
   void on_finished(int, QProcess::ExitStatus);

private:
   Git *mGit;
   bool createTemporaryFile();
   void readNewData();
   bool startProcess(QStringList args, const QString &buf = "");

   QByteArray *halfChunk = nullptr;
   UnbufferedTemporaryFile *dataFile = nullptr;
   bool canceling = false;
};

#endif
