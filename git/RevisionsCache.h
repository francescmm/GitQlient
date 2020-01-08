#pragma once

/****************************************************************************************
 ** GitQlient is an application to manage and operate one or several Git repositories. With
 ** GitQlient you will be able to add commits, branches and manage all the options Git provides.
 ** Copyright (C) 2019  Francesc Martinez
 **
 ** LinkedIn: www.linkedin.com/in/cescmm/
 ** Web: www.francescmm.com
 **
 ** This program is free software; you can redistribute it and/or
 ** modify it under the terms of the GNU Lesser General Public
 ** License as published by the Free Software Foundation; either
 ** version 2 of the License, or (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 ** Lesser General Public License for more details.
 **
 ** You should have received a copy of the GNU Lesser General Public
 ** License along with this library; if not, write to the Free Software
 ** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 ***************************************************************************************/

#include <RevisionFile.h>
#include <lanes.h>
#include <CommitInfo.h>
#include <Reference.h>

#include <QObject>
#include <QHash>

class Git;
struct WorkingDirInfo;

class RevisionsCache : public QObject
{
   Q_OBJECT

signals:
   void signalCacheUpdated();

public:
   explicit RevisionsCache(QObject *parent = nullptr);
   void configure(int numElementsToStore);
   void clear();

   int count() const { return mCommits.count(); }
   int countReferences() const { return mReferencesMap.count(); }

   CommitInfo getCommitInfoByRow(int row) const;
   CommitInfo getCommitInfo(const QString &sha) const;
   RevisionFile getRevisionFile(const QString &sha) const { return mRevisionFilesMap.value(sha); }
   Reference getReference(const QString &sha) const { return mReferencesMap.value(sha, Reference()); }

   void insertCommitInfo(CommitInfo rev);
   void insertRevisionFile(const QString &sha, const RevisionFile &file) { mRevisionFilesMap.insert(sha, file); }
   void insertReference(const QString &sha, Reference ref);
   void updateWipCommit(const QString &parentSha, const QString &diffIndex, const QString &diffIndexCache);

   void removeReference(const QString &sha) { mReferencesMap.remove(sha); }

   bool containsRevisionFile(const QString &sha) const { return mRevisionFilesMap.contains(sha); }

   RevisionFile parseDiff(const QString &sha, const QString &logDiff);
   int findFileIndex(const RevisionFile &rf, const QString &name);

   void setUntrackedFilesList(const QVector<QString> &untrackedFiles) { mUntrackedfiles = untrackedFiles; }
   bool pendingLocalChanges() const;

   uint checkRef(const QString &sha, uint mask = ANY_REF) const;
   const QStringList getRefNames(const QString &sha, uint mask) const;

private:
   bool mCacheLocked = true;
   QVector<CommitInfo *> mCommits;
   QHash<QString, CommitInfo *> mCommitsMap;
   QHash<QString, RevisionFile> mRevisionFilesMap;
   QHash<QString, Reference> mReferencesMap;
   Lanes mLanes;
   QVector<QString> mDirNames;
   QVector<QString> mFileNames;
   QVector<QString> mUntrackedfiles;

   struct FileNamesLoader
   {
      FileNamesLoader()
         : rf(nullptr)
      {
      }

      RevisionFile *rf;
      QVector<int> rfDirs;
      QVector<int> rfNames;
      QVector<QString> files;
   };

   RevisionFile fakeWorkDirRevFile(const QString &diffIndex, const QString &diffIndexCache);
   void updateLanes(CommitInfo &c);
   RevisionFile parseDiffFormat(const QString &buf, FileNamesLoader &fl);
   void appendFileName(const QString &name, FileNamesLoader &fl);
   void flushFileNames(FileNamesLoader &fl);
   void setExtStatus(RevisionFile &rf, const QString &rowSt, int parNum, FileNamesLoader &fl);
};
