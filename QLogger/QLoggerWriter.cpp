#include "QLoggerWriter.h"

#include <QDateTime>
#include <QFile>
#include <QTextStream>

namespace
{
QString levelToText(const QLogger::LogLevel &level)
{
   switch (level)
   {
      case QLogger::LogLevel::Trace:
         return "Trace";
      case QLogger::LogLevel::Debug:
         return "Debug";
      case QLogger::LogLevel::Info:
         return "Info";
      case QLogger::LogLevel::Warning:
         return "Warning";
      case QLogger::LogLevel::Error:
         return "Error";
      case QLogger::LogLevel::Fatal:
         return "Fatal";
   }

   return QString();
}
}

namespace QLogger
{

QLoggerWriter::QLoggerWriter(const QString &fileDestination, LogLevel level)
{
   mFileDestination = "logs/" + fileDestination;
   mLevel = level;
}

QString QLoggerWriter::renameFileIfFull()
{
   QFile file(mFileDestination);

   // Rename file if it's full
   if (file.size() >= MaxFileSize)
   {
      const auto newName = QString("%1_%2.log")
                               .arg(mFileDestination.left(mFileDestination.lastIndexOf('.')),
                                    QDateTime::currentDateTime().toString("dd_MM_yy__hh_mm_ss"));

      const auto renamed = file.rename(mFileDestination, newName);

      return renamed ? newName : QString();
   }

   return QString();
}

void QLoggerWriter::write(const QPair<QString, QString> &message)
{
   QFile file(mFileDestination);

   const auto prevFilename = renameFileIfFull();

   if (file.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Append))
   {
      QTextStream out(&file);

      if (!prevFilename.isEmpty())
         out << QString("%1 - Previous log %2\n").arg(message.first, prevFilename);

      out << message.second;

      file.close();
   }
}

void QLoggerWriter::enqueue(const QDateTime &date, const QString &threadId, const QString &module, LogLevel level,
                            const QString &fileName, int line, const QString &message)
{
   QString fileLine;

   if (!fileName.isEmpty() && line > 0 && mLevel <= LogLevel::Debug)
      fileLine = QString(" {%1:%2}").arg(fileName, QString::number(line));

   const auto text
       = QString("[%1] [%2] [%3] [%4]%5 %6 \n")
             .arg(levelToText(level), module, date.toString("dd-MM-yyyy hh:mm:ss.zzz"), threadId, fileLine, message);

   QMutexLocker locker(&mutex);
   messages.append({ threadId, text });

   mQueueNotEmpty.wakeOne();
}

void QLoggerWriter::run()
{
   while (!mQuit)
   {
      decltype(messages) copy;
      {
         QMutexLocker locker(&mutex);
         std::swap(copy, messages);
      }

      for (const auto &msg : qAsConst(copy))
         write(msg);

      copy.clear();

      mutex.lock();
      mQueueNotEmpty.wait(&mutex);
      mutex.unlock();
   }
}

void QLoggerWriter::closeDestination()
{
   mQuit = true;
   mQueueNotEmpty.wakeAll();
}

}
