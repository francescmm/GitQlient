#pragma once

/****************************************************************************************
 ** QLogger is a library to register and print logs into a file.
 **
 ** LinkedIn: www.linkedin.com/in/cescmm/
 ** Web: www.francescmm.com
 **
 ** This library is free software; you can redistribute it and/or
 ** modify it under the terms of the GNU Lesser General Public
 ** License as published by the Free Software Foundation; either
 ** version 2 of the License, or (at your option) any later version.
 **
 ** This library is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 ** Lesser General Public License for more details.
 **
 ** You should have received a copy of the GNU Lesser General Public
 ** License along with this library; if not, write to the Free Software
 ** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 ***************************************************************************************/

#include <QLoggerLevel.h>

#include <QMutex>
#include <QMap>
#include <QVariant>

namespace QLogger
{

class QLoggerWriter;

/**
 * @brief The QLoggerManager class manages the different destination files that we would like to have.
 */
class QLoggerManager
{
public:
   /**
    * @brief Gets an instance to the QLoggerManager.
    * @return A pointer to the instance.
    */
   static QLoggerManager *getInstance();

   /**
    * @brief This method creates a QLoogerWriter that stores the name of the file and the log
    * level assigned to it. Here is added to the map the different modules assigned to each
    * log file. The method returns <em>false</em> if a module is configured to be stored in
    * more than one file.
    *
    * @param fileDest The file name and path to print logs.
    * @param module The module that will be stored in the file.
    * @param level The maximum level allowed.
    * @return Returns true if any error have been done.
    */
   bool addDestination(const QString &fileDest, const QString &module, LogLevel level);
   /**
    * @brief This method creates a QLoogerWriter that stores the name of the file and the log
    * level assigned to it. Here is added to the map the different modules assigned to each
    * log file. The method returns <em>false</em> if a module is configured to be stored in
    * more than one file.
    *
    * @param fileDest The file name and path to print logs.
    * @param modules The modules that will be stored in the file.
    * @param level The maximum level allowed.
    * @return Returns true if any error have been done.
    */
   bool addDestination(const QString &fileDest, const QStringList &modules, LogLevel level);

   /**
    * @brief enqueueMessage Enqueues a message in the corresponding QLoggerWritter.
    * @param module The module that writes the message.
    * @param level The level of the message.
    * @param message The message to log.
    * @param file The file that logs.
    * @param line The line in the file where the log comes from.
    */
   void enqueueMessage(const QString &module, LogLevel level, const QString &message, QString file, int line);

   /**
    * @brief pause Pauses all QLoggerWriters.
    */
   void pause();

   /**
    * @brief resume Resumes all QLoggerWriters that where paused.
    */
   void resume();

   /**
    * @brief overwriteLogLevel Overwrites the log level in all the destinations
    *
    * @param level The new log level
    */
   void overwriteLogLevel(LogLevel level);

private:
   /**
    * @brief Checks if the logger is stop
    */
   bool mIsStop = false;

   /**
    * @brief Map that stores the module and the file it is assigned.
    */
   QMap<QString, QLoggerWriter *> mModuleDest;

   /**
    * @brief Defines the queue of messages when no writters have been set yet.
    */
   QMultiMap<QString, QVector<QVariant>> mNonWriterQueue;

   /**
    * @brief Mutex to make the method thread-safe.
    */
   QMutex mMutex;

   /**
    * @brief Default builder of the class. It starts the thread.
    */
   QLoggerManager();

   /**
    * @brief Destructor
    */
   ~QLoggerManager();

   /**
    * @brief Checks the queue and writes the messages if the writer is the correct one. The queue is emptied
    * for that module.
    * @param module The module to dequeue the messages from
    */
   void writeAndDequeueMessages(const QString &module);
};
}

/**
 * @brief Here is done the call to write the message in the module. First of all is confirmed
 * that the log level we want to write is less or equal to the level defined when we create the
 * destination.
 *
 * @param module The module that the message references.
 * @param level The level of the message.
 * @param message The message.
 */
void QLog_(const QString &module, QLogger::LogLevel level, const QString &message, const QString &file = QString(),
           int line = -1);

#ifndef QLog_Trace
/**
 * @brief Used to store Trace level messages.
 * @param module The module that the message references.
 * @param message The message.
 */
#   define QLog_Trace(module, message)                                                                                 \
      QLogger::QLoggerManager::getInstance()->enqueueMessage(module, QLogger::LogLevel::Trace, message, __FILE__,      \
                                                             __LINE__)
#endif

#ifndef QLog_Debug
/**
 * @brief Used to store Debug level messages.
 * @param module The module that the message references.
 * @param message The message.
 */
#   define QLog_Debug(module, message)                                                                                 \
      QLogger::QLoggerManager::getInstance()->enqueueMessage(module, QLogger::LogLevel::Debug, message, __FILE__,      \
                                                             __LINE__)
#endif

#ifndef QLog_Info
/**
 * @brief Used to store Info level messages.
 * @param module The module that the message references.
 * @param message The message.
 */
#   define QLog_Info(module, message)                                                                                  \
      QLogger::QLoggerManager::getInstance()->enqueueMessage(module, QLogger::LogLevel::Info, message, __FILE__,       \
                                                             __LINE__)
#endif

#ifndef QLog_Warning
/**
 * @brief Used to store Warning level messages.
 * @param module The module that the message references.
 * @param message The message.
 */
#   define QLog_Warning(module, message)                                                                               \
      QLogger::QLoggerManager::getInstance()->enqueueMessage(module, QLogger::LogLevel::Warning, message, __FILE__,    \
                                                             __LINE__)
#endif

#ifndef QLog_Error
/**
 * @brief Used to store Error level messages.
 * @param module The module that the message references.
 * @param message The message.
 */
#   define QLog_Error(module, message)                                                                                 \
      QLogger::QLoggerManager::getInstance()->enqueueMessage(module, QLogger::LogLevel::Error, message, __FILE__,      \
                                                             __LINE__)
#endif

#ifndef QLog_Fatal
/**
 * @brief Used to store Fatal level messages.
 * @param module The module that the message references.
 * @param message The message.
 */
#   define QLog_Fatal(module, message)                                                                                 \
      QLogger::QLoggerManager::getInstance()->enqueueMessage(module, QLogger::LogLevel::Fatal, message, __FILE__,      \
                                                             __LINE__)
#endif
