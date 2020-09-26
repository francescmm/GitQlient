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

#include <QThread>
#include <QWaitCondition>
#include <QMutex>
#include <QVector>

namespace QLogger
{

class QLoggerWriter : public QThread
{
   Q_OBJECT

public:
   /**
    * @brief Constructor that gets the complete path and filename to create the file. It also
    * configures the level of this file to filter the logs depending on the level.
    * @param fileDestination The complete path.
    * @param level The maximum level that is allowed.
    */
   explicit QLoggerWriter(const QString &fileDestination, LogLevel level);

   /**
    * @brief Gets the current level threshold.
    * @return The level.
    */
   LogLevel getLevel() const { return mLevel; }

   /**
    * @brief setLogLevel Sets the log level for this destination
    * @param level The new level threshold.
    */
   void setLogLevel(LogLevel level) { mLevel = level; }

   /**
    * @brief enqueue Enqueues a message to be written in the destiantion.
    * @param date The date and time of the log message.
    * @param threadId The thread where the message comes from.
    * @param module The module that writes the message.
    * @param level The log level of the message.
    * @param fileName The file name that prints the log.
    * @param line The line of the file name that prints the log.
    * @param message The message to log.
    */
   void enqueue(const QDateTime &date, const QString &threadId, const QString &module, LogLevel level,
                const QString &fileName, int line, const QString &message);

   /**
    * @brief Stops the log writer
    * @param stop True to be stop, otherwise false
    */
   void stop(bool stop) { mIsStop = stop; }

   /**
    * @brief Returns if the log writer is stop from writing.
    * @return True if is stop, otherwise false
    */
   bool isStop() const { return mIsStop; }

   void run() override;

   void closeDestination();

private:
   bool mQuit = false;
   bool mIsStop = false;
   QWaitCondition mQueueNotEmpty;
   QString mFileDestination;
   LogLevel mLevel;
   QVector<QPair<QString, QString>> messages;
   QMutex mutex;
   static const int MaxFileSize = 1024 * 1024;

   /**
    * @brief renameFileIfFull Truncates the log file in two. Keeps the filename for the new one and renames the old one
    * with the timestamp.
    *
    * @return Returns the file name for the old logs.
    */
   QString renameFileIfFull();

   /**
    * @brief Writes a message in a file. If the file is full, it truncates it and prints a first line with the
    * information of the old file.
    *
    * @param message Pair of values consistent on the date and the message to be log.
    */
   void write(const QPair<QString, QString> &message);
};

}
