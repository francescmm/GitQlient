#pragma once

#include <QByteArray>
#include <QVector>

class RevisionFile
{
public:
   enum StatusFlag
   {
      MODIFIED = 1,
      DELETED = 2,
      NEW = 4,
      RENAMED = 8,
      COPIED = 16,
      UNKNOWN = 32,
      IN_INDEX = 64,
      CONFLICT = 128
   };

   RevisionFile() = default;

   /* This QByteArray keeps indices in some dir and names vectors,
    * defined outside RevisionFile. Paths are splitted in dir and file
    * name, first all the dirs are listed then the file names to
    * achieve a better compression when saved to disk.
    * A single QByteArray is used instead of two vectors because it's
    * much faster to load from disk when using a QDataStream
    */
   QByteArray pathsIdx;

   int dirAt(int idx) const { return ((const int *)pathsIdx.constData())[idx]; }
   int nameAt(int idx) const { return ((const int *)pathsIdx.constData())[count() + idx]; }

   QVector<int> mergeParent;

   // helper functions
   int count() const { return pathsIdx.size() / (sizeof(int) * 2); }
   bool statusCmp(int idx, StatusFlag sf) const;
   const QString extendedStatus(int idx) const;
   void setStatus(const QString &rowSt);
   void setStatus(RevisionFile::StatusFlag flag);
   void setStatus(int pos, RevisionFile::StatusFlag flag);
   void appendStatus(int pos, RevisionFile::StatusFlag flag);
   int getStatus(int pos) const { return status.at(pos); }
   void setOnlyModified(bool onlyModified) { mOnlyModified = onlyModified; }
   int getFilesCount() const { return status.size(); }
   void appendExtStatus(const QString &file) { extStatus.append(file); }

private:
   // friend class Git;

   // Status information is splitted in a flags vector and in a string
   // vector in 'status' are stored flags according to the info returned
   // by 'git diff-tree' without -C option.
   // In case of a working directory file an IN_INDEX flag is or-ed togheter in
   // case file is present in git index.
   // If file is renamed or copied an entry in 'extStatus' stores the
   // value returned by 'git diff-tree -C' plus source and destination
   // files info.
   // When status of all the files is 'modified' then onlyModified is
   // set, this let us to do some optimization in this common case
   bool mOnlyModified = true;
   QVector<int> status;
   QVector<QString> extStatus;
};
