/*
        Description: interface to git programs

        Author: Marco Costalba (C) 2005-2007

        Copyright: See COPYING file that comes with this distribution

*/

#include <QDataStream>
#include <QTextDocument>
#include "common.h"

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
