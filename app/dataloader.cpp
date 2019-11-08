/*
        Description: async stream reader, used to load repository data on startup

        Author: Marco Costalba (C) 2005-2007

        Copyright: See COPYING file that comes with this distribution

*/
#include "git.h"
#include "dataloader.h"

#include <QDir>
#include <QTemporaryFile>
#include <QTextStream>
#include <QDebug>

#define GUI_UPDATE_INTERVAL 500
#define READ_BLOCK_SIZE 65535

class UnbufferedTemporaryFile : public QTemporaryFile
{
public:
   explicit UnbufferedTemporaryFile(QObject *p)
      : QTemporaryFile(p)
   {
   }
   bool unbufOpen() { return open(QIODevice::ReadOnly | QIODevice::Unbuffered); }
};

DataLoader::DataLoader(Git *git)
   : QProcess()
   , mGit(git)
{
   connect(mGit, &Git::cancelAllProcesses, this, &DataLoader::on_cancel);
}

DataLoader::~DataLoader()
{
   waitForFinished(1000);
}

void DataLoader::on_cancel()
{
   if (!canceling)
   {
      canceling = true;
      kill();
   }
}

bool DataLoader::start(const QStringList &args, const QString &wd, const QString &buf)
{
   setWorkingDirectory(wd);

   connect(this, qOverload<int, QProcess::ExitStatus>(&DataLoader::finished), this, &DataLoader::on_finished);

   if (!createTemporaryFile() || !startProcess(args, buf))
   {
      readAll();
      deleteLater();
      return false;
   }

   return true;
}

void DataLoader::on_finished(int, QProcess::ExitStatus)
{
   if (canceling)
   {
      deleteLater();
      return; // we leave with guiUpdateTimer not active
   }

   readNewData();

   emit newDataReady(); // inserting in list view is about 3% of total time
   emit loaded();

   deleteLater();
}

void DataLoader::readNewData()
{
   bool ok = dataFile && (dataFile->isOpen() || (dataFile->exists() && dataFile->unbufOpen()));

   if (ok)
   {
      const auto ba = dataFile->readAll();

      if (ba.size() != 0 && !canceling)
         mGit->addChunk(ba);
   }
}

bool DataLoader::createTemporaryFile()
{
   // redirect 'git log' output to a temporary file
   dataFile = new UnbufferedTemporaryFile(this);

   if (!dataFile->open()) // to read the file name
      return false;

   setStandardOutputFile(dataFile->fileName());
   dataFile->close();
   return true;
}

bool DataLoader::startProcess(QStringList args, const QString &buf)
{
   if (args.isEmpty())
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
      QTemporaryFile *bufFile = new QTemporaryFile(this);
      bufFile->open();
      QTextStream stream(bufFile);
      stream << buf;
      setStandardInputFile(bufFile->fileName());
      bufFile->close();
   }

   QStringList env = QProcess::systemEnvironment();
   env << "GIT_TRACE=0"; // avoid choking on debug traces
   env << "GIT_FLUSH=0"; // skip the fflush() in 'git log'
   setEnvironment(env);

   QProcess::start(prog, arguments); // TODO test QIODevice::Unbuffered
   return waitForStarted();
}
