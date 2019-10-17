#include "RevisionFile.h"
#include <QDataStream>

/**
 * RevisionFile streaming out
 */
const RevisionFile &RevisionFile::operator>>(QDataStream &stream) const
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
 * RevisionFile streaming in
 */
RevisionFile &RevisionFile::operator<<(QDataStream &stream)
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
