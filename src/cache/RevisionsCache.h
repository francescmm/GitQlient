#pragma once

/****************************************************************************************
 ** GitQlient is an application to manage and operate one or several Git repositories. With
 ** GitQlient you will be able to add commits, branches and manage all the options Git provides.
 ** Copyright (C) 2020  Francesc Martinez
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

#include <RevisionFiles.h>
#include <lanes.h>
#include <CommitInfo.h>

#include <QObject>
#include <QHash>

struct WorkingDirInfo;

class RevisionsCache : public QObject
{
   Q_OBJECT

signals:
   void signalCacheUpdated();

public:
   struct LocalBranchDistances
   {
      int aheadMaster = 0;
      int behindMaster = 0;
      int aheadOrigin = 0;
      int behindOrigin = 0;
   };

   explicit RevisionsCache(QObject *parent = nullptr);
   ~RevisionsCache();

   void configure(int numElementsToStore);
   void clear();

   int count() const;

   CommitInfo getCommitInfo(const QString &sha) const;
   CommitInfo getCommitInfoByRow(int row) const;
   int getCommitPos(const QString &sha) const;
   CommitInfo getCommitInfoByField(CommitInfo::Field field, const QString &text, int startingPoint = 0);
   RevisionFiles getRevisionFile(const QString &sha1, const QString &sha2) const;

   void insertCommitInfo(CommitInfo rev, int orderIdx);

   bool insertRevisionFile(const QString &sha1, const QString &sha2, const RevisionFiles &file);
   void insertReference(const QString &sha, References::Type type, const QString &reference);
   void insertLocalBranchDistances(const QString &name, const LocalBranchDistances &distances);
   LocalBranchDistances getLocalBranchDistances(const QString &name) { return mLocalBranchDistances.value(name); }
   void updateWipCommit(const QString &parentSha, const QString &diffIndex, const QString &diffIndexCache);

   void removeReference(const QString &sha);

   bool containsRevisionFile(const QString &sha1, const QString &sha2) const;

   RevisionFiles parseDiff(const QString &logDiff);

   void setUntrackedFilesList(const QVector<QString> &untrackedFiles);
   bool pendingLocalChanges() const;

   QVector<QPair<QString, QStringList>> getBranches(References::Type type) const;
   QVector<QPair<QString, QStringList>> getTags() const;

   QString getCommitForBranch(const QString &branch, bool local = true) const;

private:
   bool mCacheLocked = true;
   QVector<CommitInfo *> mCommits;
   QHash<QString, CommitInfo *> mCommitsMap;
   QHash<QPair<QString, QString>, RevisionFiles> mRevisionFilesMap;
   QVector<CommitInfo *> mReferences;
   QMap<QString, LocalBranchDistances> mLocalBranchDistances;
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

      RevisionFiles *rf;
      QVector<int> rfDirs;
      QVector<int> rfNames;
      QVector<QString> files;
   };

   RevisionFiles fakeWorkDirRevFile(const QString &diffIndex, const QString &diffIndexCache);
   QVector<Lane> calculateLanes(const CommitInfo &c);
   RevisionFiles parseDiffFormat(const QString &buf, FileNamesLoader &fl);
   void appendFileName(const QString &name, FileNamesLoader &fl);
   void flushFileNames(FileNamesLoader &fl);
   void setExtStatus(RevisionFiles &rf, const QString &rowSt, int parNum, FileNamesLoader &fl);
   QVector<CommitInfo *>::const_iterator searchCommit(CommitInfo::Field field, const QString &text,
                                                      int startingPoint = 0) const;
   void resetLanes(const CommitInfo &c, bool isFork);
};
