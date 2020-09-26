#include <QLogger.h>

#include "QLoggerWriter.h"

#include <QDir>
#include <QDateTime>

Q_DECLARE_METATYPE(QLogger::LogLevel);

namespace QLogger
{

void QLog_(const QString &module, LogLevel level, const QString &message, const QString &file, int line)
{
   QLoggerManager::getInstance()->enqueueMessage(module, level, message, file, line);
}

static const int QUEUE_LIMIT = 100;

QLoggerManager::QLoggerManager()
   : mMutex(QMutex::Recursive)
{
   QDir dir(QDir::currentPath());
   dir.mkdir("logs");
}

QLoggerManager *QLoggerManager::getInstance()
{
   static QLoggerManager INSTANCE;

   return &INSTANCE;
}

bool QLoggerManager::addDestination(const QString &fileDest, const QString &module, LogLevel level)
{
   QMutexLocker lock(&mMutex);

   if (!mModuleDest.contains(module))
   {
      const auto log = new QLoggerWriter(fileDest, level);
      log->stop(mIsStop);

      const auto threadId = QString("%1").arg((quintptr)QThread::currentThread(), QT_POINTER_SIZE * 2, 16, QChar('0'));

      log->enqueue(QDateTime::currentDateTime(), threadId, module, LogLevel::Info, "", -1, "Adding destination!");

      mModuleDest.insert(module, log);

      log->start();

      return true;
   }

   return false;
}

bool QLoggerManager::addDestination(const QString &fileDest, const QStringList &modules, LogLevel level)
{
   QMutexLocker lock(&mMutex);
   bool allAdded = false;

   for (const auto &module : modules)
   {
      if (!mModuleDest.contains(module))
      {
         const auto log = new QLoggerWriter(fileDest, level);
         log->stop(mIsStop);

         const auto threadId
             = QString("%1").arg((quintptr)QThread::currentThread(), QT_POINTER_SIZE * 2, 16, QChar('0'));

         log->enqueue(QDateTime::currentDateTime(), threadId, module, LogLevel::Info, "", -1, "Adding destination!");

         mModuleDest.insert(module, log);

         log->start();

         allAdded = true;
      }
   }

   return allAdded;
}

void QLoggerManager::writeAndDequeueMessages(const QString &module)
{
   QMutexLocker lock(&mMutex);

   const auto logWriter = mModuleDest.value(module);

   if (logWriter && !logWriter->isStop())
   {
      for (const auto &values : qAsConst(mNonWriterQueue))
      {
         const auto level = qvariant_cast<LogLevel>(values.at(2).toInt());

         if (logWriter->getLevel() <= level)
         {
            const auto datetime = values.at(0).toDateTime();
            const auto threadId = values.at(1).toString();
            const auto file = values.at(3).toString();
            const auto line = values.at(4).toInt();
            const auto message = values.at(5).toString();

            logWriter->enqueue(datetime, threadId, module, level, file, line, message);
         }
      }

      mNonWriterQueue.remove(module);
   }
}

void QLoggerManager::enqueueMessage(const QString &module, LogLevel level, const QString &message, QString file,
                                    int line)
{
   QMutexLocker lock(&mMutex);
   const auto threadId = QString("%1").arg((quintptr)QThread::currentThread(), QT_POINTER_SIZE * 2, 16, QChar('0'));
   const auto fileName = file.mid(file.lastIndexOf('/') + 1);
   const auto logWriter = mModuleDest.value(module);

   if (logWriter && !logWriter->isStop() && logWriter->getLevel() <= level)
   {
      writeAndDequeueMessages(module);

      logWriter->enqueue(QDateTime::currentDateTime(), threadId, module, level, fileName, line, message);
   }
   else if (!logWriter && mNonWriterQueue.count(module) < QUEUE_LIMIT)
      mNonWriterQueue.insert(
          module,
          { QDateTime::currentDateTime(), threadId, QVariant::fromValue<LogLevel>(level), fileName, line, message });
}

void QLoggerManager::pause()
{
   QMutexLocker lock(&mMutex);

   mIsStop = true;

   for (auto &logWriter : mModuleDest)
      logWriter->stop(mIsStop);
}

void QLoggerManager::resume()
{
   QMutexLocker lock(&mMutex);

   mIsStop = false;

   for (auto &logWriter : mModuleDest)
      logWriter->stop(mIsStop);
}

void QLoggerManager::overwriteLogLevel(LogLevel level)
{
   QMutexLocker lock(&mMutex);

   for (auto &logWriter : mModuleDest)
      logWriter->setLogLevel(level);
}

QLoggerManager::~QLoggerManager()
{
   QMutexLocker locker(&mMutex);

   for (const auto &dest : mModuleDest.toStdMap())
      writeAndDequeueMessages(dest.first);

   for (auto dest : qAsConst(mModuleDest))
   {
      dest->closeDestination();
      dest->deleteLater();
   }

   mModuleDest.clear();
}

}
