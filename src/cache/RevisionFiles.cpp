#include "RevisionFiles.h"

bool RevisionFiles::operator==(const RevisionFiles &revFiles) const
{
   return mFiles == revFiles.mFiles && mOnlyModified == revFiles.mOnlyModified && mergeParent == revFiles.mergeParent
       && mFileStatus == revFiles.mFileStatus && mRenamedFiles == revFiles.mRenamedFiles;
}

bool RevisionFiles::operator!=(const RevisionFiles &revFiles) const
{
   return !(*this == revFiles);
}

bool RevisionFiles::statusCmp(int idx, RevisionFiles::StatusFlag sf) const
{
   if (idx >= mFileStatus.count())
      return false;

   return (mOnlyModified ? MODIFIED : mFileStatus.at(static_cast<int>(idx))) & sf;
}

const QString RevisionFiles::extendedStatus(int idx) const
{
   /*
         rf.extStatus has size equal to position of latest copied/renamed file,
         that could be lower then count(), so we have to explicitly check for
         an out of bound condition.
      */
   return !mRenamedFiles.isEmpty() && idx < mRenamedFiles.count() ? mRenamedFiles.at(idx) : "";
}

void RevisionFiles::setStatus(const QString &rowSt, bool isStaged)
{
   switch (rowSt.at(0).toLatin1())
   {
      case 'M':
      case 'T':
         mFileStatus.append(RevisionFiles::MODIFIED);
         if (isStaged)
            mFileStatus[mFileStatus.count() - 1] |= RevisionFiles::IN_INDEX;
         break;
      case 'U':
         mFileStatus.append(RevisionFiles::MODIFIED);
         mFileStatus[mFileStatus.count() - 1] |= RevisionFiles::CONFLICT;
         if (isStaged)
            mFileStatus[mFileStatus.count() - 1] |= RevisionFiles::IN_INDEX;
         mOnlyModified = false;
         break;
      case 'D':
         mFileStatus.append(RevisionFiles::DELETED);
         mOnlyModified = false;
         if (isStaged)
            mFileStatus[mFileStatus.count() - 1] |= RevisionFiles::IN_INDEX;
         break;
      case 'A':
         mFileStatus.append(RevisionFiles::NEW);
         mOnlyModified = false;
         if (isStaged)
            mFileStatus[mFileStatus.count() - 1] |= RevisionFiles::IN_INDEX;
         break;
      case '?':
         mFileStatus.append(RevisionFiles::UNKNOWN);
         mOnlyModified = false;
         break;
      default:
         mFileStatus.append(RevisionFiles::MODIFIED);
         break;
   }
}

void RevisionFiles::setStatus(RevisionFiles::StatusFlag flag)
{
   mFileStatus.append(flag);

   if (flag == RevisionFiles::DELETED || flag == RevisionFiles::NEW || flag == RevisionFiles::UNKNOWN)
      mOnlyModified = false;
}

void RevisionFiles::setStatus(int pos, RevisionFiles::StatusFlag flag)
{
   mFileStatus[pos] = flag;
}

void RevisionFiles::appendStatus(int pos, RevisionFiles::StatusFlag flag)
{
   mFileStatus[pos] |= flag;
}
