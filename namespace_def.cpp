/*
        Author: Marco Costalba (C) 2005-2007

        Copyright: See COPYING file that comes with this distribution


 Definitions of complex namespace constants

 Complex constant objects are not folded in like integral types, so they
 are declared 'extern' in namespace to avoid duplicating them as file scope
 data in each file where QGit namespace is included.

*/
#include <QDir>
#include <QHash>
#include <QPixmap>
#include <QProcess>
#include <QSettings>
#include <QSplitter>
#include <QTemporaryFile>
#include <QTextStream>
#include <QWidget>
#include <QTextCodec>
#include "common.h"
#include "git.h"

#include <sys/types.h> // used by chmod()
#include <sys/stat.h> // used by chmod()

const QString QGit::SCRIPT_EXT = ".sh";

// definition of an optimized sha hash function
static inline uint hexVal(const uchar *ch)
{

   return (*ch < 64 ? *ch - 48 : *ch - 87);
}

uint qHash(const QString &s)
{ // fast path, called 6-7 times per revision

   const uchar *ch = reinterpret_cast<const uchar *>(s.data());
   return (hexVal(ch) << 24) + (hexVal(ch + 2) << 20) + (hexVal(ch + 4) << 16) + (hexVal(ch + 6) << 12)
       + (hexVal(ch + 8) << 8) + (hexVal(ch + 10) << 4) + hexVal(ch + 12);
}

const QString QGit::toPersistentSha(const QString &sha, QVector<QByteArray> &v)
{

   v.append(sha.toLatin1());
   return QString::fromUtf8(v.last().constData());
}

// initialized at startup according to system wide settings
QString QGit::GIT_DIR;

// git index parameters
const QString QGit::ZERO_SHA = "0000000000000000000000000000000000000000";
const QString QGit::CUSTOM_SHA = "*** CUSTOM * CUSTOM * CUSTOM * CUSTOM **";
const QString QGit::ALL_MERGE_FILES = "ALL_MERGE_FILES";

// cache file
const QString QGit::BAK_EXT = ".bak";
const QString QGit::C_DAT_FILE = "/qgit_cache.dat";

using namespace QGit;

// misc helpers
bool QGit::stripPartialParaghraps(const QByteArray &ba, QString *dst, QString *prev)
{

   QTextCodec *tc = QTextCodec::codecForLocale();

   if (ba.endsWith('\n'))
   { // optimize common case
      *dst = tc->toUnicode(ba);

      // handle rare case of a '\0' inside content
      while (dst->size() < ba.size() && ba.at(dst->size()) == '\0')
      {
         QString s = tc->toUnicode(ba.mid(dst->size() + 1)); // sizes should match
         dst->append(" ").append(s);
      }

      dst->truncate(dst->size() - 1); // strip trailing '\n'
      if (!prev->isEmpty())
      {
         dst->prepend(*prev);
         prev->clear();
      }
      return true;
   }
   QString src = tc->toUnicode(ba);
   // handle rare case of a '\0' inside content
   while (src.size() < ba.size() && ba.at(src.size()) == '\0')
   {
      QString s = tc->toUnicode(ba.mid(src.size() + 1));
      src.append(" ").append(s);
   }

   int idx = src.lastIndexOf('\n');
   if (idx == -1)
   {
      prev->append(src);
      dst->clear();
      return false;
   }
   *dst = src.left(idx).prepend(*prev); // strip trailing '\n'
   *prev = src.mid(idx + 1); // src[idx] is '\n', skip it
   return true;
}

bool QGit::writeToFile(const QString &fileName, const QString &data, bool setExecutable)
{

   QFile file(fileName);
   if (!file.open(QIODevice::WriteOnly))
      return false;

   QString data2(data);
   QTextStream stream(&file);

#ifdef Q_OS_WIN32
   data2.replace("\r\n", "\n"); // change windows CRLF to linux
   data2.replace("\n", "\r\n"); // then change all linux CRLF to windows
#endif
   stream << data2;
   file.close();

#ifndef Q_OS_WIN32
   if (setExecutable)
      chmod(fileName.toLatin1().constData(), 0755);
#endif
   return true;
}

bool QGit::writeToFile(const QString &fileName, const QByteArray &data, bool setExecutable)
{

   QFile file(fileName);
   if (!file.open(QIODevice::WriteOnly))
      return false;

   QDataStream stream(&file);
   stream.writeRawData(data.constData(), data.size());
   file.close();

#ifndef Q_OS_WIN32
   if (setExecutable)
      chmod(fileName.toLatin1().constData(), 0755);
#endif
   return true;
}

bool QGit::readFromFile(const QString &fileName, QString &data)
{

   data = "";
   QFile file(fileName);
   if (!file.open(QIODevice::ReadOnly))
      return false;

   QTextStream stream(&file);
   data = stream.readAll();
   file.close();
   return true;
}
