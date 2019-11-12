#include "RevisionFile.h"

bool RevisionFile::statusCmp(int idx, RevisionFile::StatusFlag sf) const
{
   return (onlyModified ? MODIFIED : status.at(static_cast<int>(idx))) & sf;
}

const QString RevisionFile::extendedStatus(int idx) const
{
   /*
         rf.extStatus has size equal to position of latest copied/renamed file,
         that could be lower then count(), so we have to explicitly check for
         an out of bound condition.
      */
   return !extStatus.isEmpty() && idx < extStatus.count() ? extStatus.at(idx) : "";
}

void RevisionFile::setStatus(const QString &rowSt)
{
   switch (rowSt.at(0).toLatin1())
   {
      case 'M':
      case 'T':
      case 'U':
         status.append(RevisionFile::MODIFIED);
         break;
      case 'D':
         status.append(RevisionFile::DELETED);
         onlyModified = false;
         break;
      case 'A':
         status.append(RevisionFile::NEW);
         onlyModified = false;
         break;
      case '?':
         status.append(RevisionFile::UNKNOWN);
         onlyModified = false;
         break;
      default:
         status.append(RevisionFile::MODIFIED);
         break;
   }
}
