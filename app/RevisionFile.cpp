#include "RevisionFile.h"

bool RevisionFile::statusCmp(int idx, RevisionFile::StatusFlag sf) const
{
   return (mOnlyModified ? MODIFIED : status.at(static_cast<int>(idx))) & sf;
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
         status.append(RevisionFile::MODIFIED);
         break;
      case 'U':
         status.append(RevisionFile::MODIFIED);
         status[status.count() - 1] |= RevisionFile::CONFLICT;
         mOnlyModified = false;
         break;
      case 'D':
         status.append(RevisionFile::DELETED);
         mOnlyModified = false;
         break;
      case 'A':
         status.append(RevisionFile::NEW);
         mOnlyModified = false;
         break;
      case '?':
         status.append(RevisionFile::UNKNOWN);
         mOnlyModified = false;
         break;
      default:
         status.append(RevisionFile::MODIFIED);
         break;
   }
}

void RevisionFile::setStatus(RevisionFile::StatusFlag flag)
{
   status.append(flag);

   if (flag == RevisionFile::DELETED || flag == RevisionFile::NEW || flag == RevisionFile::UNKNOWN)
      mOnlyModified = false;
}

void RevisionFile::setStatus(int pos, RevisionFile::StatusFlag flag)
{
   status[pos] = flag;
}

void RevisionFile::appendStatus(int pos, RevisionFile::StatusFlag flag)
{
   status[pos] |= flag;
}
