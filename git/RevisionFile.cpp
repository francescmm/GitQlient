#include "RevisionFile.h"

bool RevisionFile::statusCmp(int idx, RevisionFile::StatusFlag sf) const
{
   if (idx >= mFileStatus.count())
      return false;

   return (mOnlyModified ? MODIFIED : mFileStatus.at(static_cast<int>(idx))) & sf;
}

const QString RevisionFile::extendedStatus(int idx) const
{
   /*
         rf.extStatus has size equal to position of latest copied/renamed file,
         that could be lower then count(), so we have to explicitly check for
         an out of bound condition.
      */
   return !mRenamedFiles.isEmpty() && idx < mRenamedFiles.count() ? mRenamedFiles.at(idx) : "";
}

void RevisionFile::setStatus(const QString &rowSt)
{
   switch (rowSt.at(0).toLatin1())
   {
      case 'M':
      case 'T':
         mFileStatus.append(RevisionFile::MODIFIED);
         break;
      case 'U':
         mFileStatus.append(RevisionFile::MODIFIED);
         mFileStatus[mFileStatus.count() - 1] |= RevisionFile::CONFLICT;
         mOnlyModified = false;
         break;
      case 'D':
         mFileStatus.append(RevisionFile::DELETED);
         mOnlyModified = false;
         break;
      case 'A':
         mFileStatus.append(RevisionFile::NEW);
         mOnlyModified = false;
         break;
      case '?':
         mFileStatus.append(RevisionFile::UNKNOWN);
         mOnlyModified = false;
         break;
      default:
         mFileStatus.append(RevisionFile::MODIFIED);
         break;
   }
}

void RevisionFile::setStatus(RevisionFile::StatusFlag flag)
{
   mFileStatus.append(flag);

   if (flag == RevisionFile::DELETED || flag == RevisionFile::NEW || flag == RevisionFile::UNKNOWN)
      mOnlyModified = false;
}

void RevisionFile::setStatus(int pos, RevisionFile::StatusFlag flag)
{
   mFileStatus[pos] = flag;
}

void RevisionFile::appendStatus(int pos, RevisionFile::StatusFlag flag)
{
   mFileStatus[pos] |= flag;
}
