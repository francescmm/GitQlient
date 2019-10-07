/*
        Description: interface to git programs

        Author: Marco Costalba (C) 2005-2007

        Copyright: See COPYING file that comes with this distribution

*/

#include <QDataStream>
#include <QTextDocument>
#include "common.h"

const QString Rev::mid(int start, int len) const
{

   // warning no sanity check is done on arguments
   const char *data = ba.constData();
   return QString::fromLocal8Bit(data + start, len);
}

const QString Rev::midSha(int start, int len) const
{

   // warning no sanity check is done on arguments
   const char *data = ba.constData();
   return QString::fromLatin1(data + start, len); // faster then formAscii
}

const QString Rev::parent(int idx) const
{

   return QString::fromUtf8(ba.constData() + shaStart + 41 + 41 * idx);
}

const QStringList Rev::parents() const
{

   QStringList p;
   int idx = shaStart + 41;

   for (int i = 0; i < parentsCnt; i++)
   {
      p.append(midSha(idx, 40));
      idx += 41;
   }
   return p;
}

int Rev::indexData(bool quick, bool withDiff) const
{
   /*
 This is what 'git log' produces:

       - a possible one line with "Final output:\n" in case of --early-output option
       - one line with "log size" + len of this record
       - one line with boundary info + sha + an arbitrary amount of parent's sha
       - one line with committer name + e-mail
       - one line with author name + e-mail
       - one line with author date as unix timestamp
       - zero or more non blank lines with other info, as the encoding FIXME
       - one blank line
       - zero or one line with log title
       - zero or more lines with log message
       - zero or more lines with diff content (only for file history)
       - a terminating '\0'
*/
   static int error = -1;
   static int shaLength = 40; // from git ref. spec.
   static int shaEndlLength = shaLength + 1; // an sha key + \n
   static int shaXEndlLength = shaLength + 2; // an sha key + X marker + \n
   static char finalOutputMarker = 'F'; // marks the beginning of "Final output" string
   static char logSizeMarker = 'l'; // marks the beginning of "log size" string
   static int logSizeStrLength = 9; // "log size"
   static int asciiPosOfZeroChar = 48; // char "0" has value 48 in ascii table

   const int last = ba.size() - 1;
   int logSize = 0, idx = start;
   int logEnd, revEnd;

   // direct access is faster then QByteArray.at()
   const char *data = ba.constData();
   char *fixup = const_cast<char *>(data); // to build '\0' terminating strings

   if (start + shaXEndlLength > last) // at least sha header must be present
      return -1;

   if (data[start] == finalOutputMarker) // "Final output", let caller handle this
      return (ba.indexOf('\n', start) != -1 ? -2 : -1);

   // parse   'log size xxx\n'   if present -- from git ref. spec.
   if (data[idx] == logSizeMarker)
   {
      idx += logSizeStrLength; // move idx to beginning of log size value

      // parse log size value
      int digit;
      while ((digit = data[idx++]) != '\n')
         logSize = logSize * 10 + digit - asciiPosOfZeroChar;
   }
   // idx points to the boundary information, which has the same length as an sha header.
   if (++idx + shaXEndlLength > last)
      return error;

   shaStart = idx;

   // ok, now shaStart is valid but msgSize could be still 0 if not available
   logEnd = shaStart - 1 + logSize;
   if (logEnd > last)
      return error;

   idx += shaLength; // now points to 'X' place holder

   fixup[idx] = '\0'; // we want sha to be a '\0' terminated ascii string

   parentsCnt = 0;

   if (data[idx + 2] == '\n') // initial revision
      ++idx;
   else
      do
      {
         parentsCnt++;
         idx += shaEndlLength;

         if (idx + 1 >= last)
            break;

         fixup[idx] = '\0'; // we want parents '\0' terminated

      } while (data[idx + 1] != '\n');

   ++idx; // now points to the trailing '\n' of sha line

   // check for !msgSize
   if (withDiff || !logSize)
   {

      revEnd = (logEnd > idx) ? logEnd - 1 : idx;
      revEnd = ba.indexOf('\0', revEnd + 1);
      if (revEnd == -1)
         return -1;
   }
   else
      revEnd = logEnd;

   if (revEnd > last) // after this point we know to have the whole record
      return error;

   // ok, now revEnd is valid but logEnd could be not if !logSize
   // in case of diff we are sure content will be consumed so
   // we go all the way
   if (quick && !withDiff)
      return ++revEnd;

   // commiter
   comStart = ++idx;
   idx = ba.indexOf('\n', idx); // committer line end
   if (idx == -1)
      return -1;

   // author
   autStart = ++idx;
   idx = ba.indexOf('\n', idx); // author line end
   if (idx == -1)
      return -1;

   // author date in Unix format (seconds since epoch)
   autDateStart = ++idx;
   idx = ba.indexOf('\n', idx); // author date end without '\n'
   if (idx == -1)
      return -1;

   // if no error, point to trailing \n
   ++idx;

   diffStart = diffLen = 0;
   if (withDiff)
   {
      diffStart = logSize ? logEnd : ba.indexOf("\ndiff ", idx);

      if (diffStart != -1 && diffStart < revEnd)
         diffLen = revEnd - ++diffStart;
      else
         diffStart = 0;
   }
   if (!logSize)
      logEnd = diffStart ? diffStart : revEnd;

   // ok, now logEnd is valid and we can handle the log
   sLogStart = idx;

   if (logEnd < sLogStart)
   { // no shortlog no longLog

      sLogStart = sLogLen = 0;
      lLogStart = lLogLen = 0;
   }
   else
   {
      lLogStart = ba.indexOf('\n', sLogStart);
      if (lLogStart != -1 && lLogStart < logEnd - 1)
      {

         sLogLen = lLogStart - sLogStart; // skip sLog trailing '\n'
         lLogLen = logEnd - lLogStart; // include heading '\n' in long log
      }
      else
      { // no longLog
         sLogLen = logEnd - sLogStart;
         if (data[sLogStart + sLogLen - 1] == '\n')
            sLogLen--; // skip trailing '\n' if any

         lLogStart = lLogLen = 0;
      }
   }
   indexed = true;
   return ++revEnd;
}

/**
 * RevFile streaming out
 */
const RevFile &RevFile::operator>>(QDataStream &stream) const
{

   stream << pathsIdx;

   // skip common case of only modified files
   bool isEmpty = onlyModified;
   stream << (quint32)isEmpty;
   if (!isEmpty)
      stream << status;

   // skip common case of just one parent
   isEmpty = (mergeParent.isEmpty() || mergeParent.last() == 1);
   stream << (quint32)isEmpty;
   if (!isEmpty)
      stream << mergeParent;

   // skip common case of no rename/copies
   isEmpty = extStatus.isEmpty();
   stream << (quint32)isEmpty;
   if (!isEmpty)
      stream << extStatus;

   return *this;
}

/**
 * RevFile streaming in
 */
RevFile &RevFile::operator<<(QDataStream &stream)
{

   stream >> pathsIdx;

   bool isEmpty;
   quint32 tmp;

   stream >> tmp;
   onlyModified = (bool)tmp;
   if (!onlyModified)
      stream >> status;

   stream >> tmp;
   isEmpty = (bool)tmp;
   if (!isEmpty)
      stream >> mergeParent;

   stream >> tmp;
   isEmpty = (bool)tmp;
   if (!isEmpty)
      stream >> extStatus;

   return *this;
}

QString qt4and5escaping(QString toescape)
{
#if QT_VERSION >= 0x050000
   return toescape.toHtmlEscaped();
#else
   return Qt::escape(toescape);
#endif
}
