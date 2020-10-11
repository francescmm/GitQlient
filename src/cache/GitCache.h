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

#include <QSharedPointer>
#include <QObject>
#include <QHash>
#include <QMutex>

struct WipRevisionInfo
{
   QString parentSha;
   QString diffIndex;
   QString diffIndexCached;

   bool isValid() const { return !parentSha.isEmpty() || !diffIndex.isEmpty() || !diffIndexCached.isEmpty(); }
};

class GitCache : public QObject
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

   explicit GitCache(QObject *parent = nullptr);
   ~GitCache();

   void setup(const WipRevisionInfo &wipInfo, const QList<CommitInfo> &commits);

   int count() const;

   CommitInfo getCommitInfo(const QString &sha);
   CommitInfo getCommitInfoByRow(int row);
   int getCommitPos(const QString &sha);
   CommitInfo getCommitInfoByField(CommitInfo::Field field, const QString &text, int startingPoint, bool reverse);
   RevisionFiles getRevisionFile(const QString &sha1, const QString &sha2) const;

   bool insertRevisionFile(const QString &sha1, const QString &sha2, const RevisionFiles &file);
   void insertReference(const QString &sha, References::Type type, const QString &reference);
   void insertLocalBranchDistances(const QString &name, const LocalBranchDistances &distances);
   LocalBranchDistances getLocalBranchDistances(const QString &name) { return mLocalBranchDistances.value(name); }
   void updateWipCommit(const QString &parentSha, const QString &diffIndex, const QString &diffIndexCache);

   bool containsRevisionFile(const QString &sha1, const QString &sha2) const;

   RevisionFiles parseDiff(const QString &logDiff);

   void setUntrackedFilesList(const QVector<QString> &untrackedFiles);
   bool pendingLocalChanges();

   QVector<QPair<QString, QStringList>> getBranches(References::Type type);
   QMap<QString, QString> getTags(References::Type tagType) const;

   void updateTags(const QMap<QString, QString> &remoteTags);

private:
   friend class GitRepoLoader;

   QMutex mMutex;
   bool mConfigured = true;
   QVector<CommitInfo *> mCommits;
   QHash<QString, CommitInfo> mCommitsMap;
   QMultiMap<QString, CommitInfo *> mTmpChildsStorage;
   QHash<QPair<QString, QString>, RevisionFiles> mRevisionFilesMap;
   QVector<CommitInfo *> mReferences;
   QMap<QString, LocalBranchDistances> mLocalBranchDistances;
   Lanes mLanes;
   QVector<QString> mDirNames;
   QVector<QString> mFileNames;
   QVector<QString> mUntrackedfiles;
   QMap<QString, QString> mRemoteTags;

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

   void setConfigurationDone() { mConfigured = true; }
   void insertCommitInfo(CommitInfo rev, int orderIdx);
   void insertWipRevision(const QString &parentSha, const QString &diffIndex, const QString &diffIndexCache);
   RevisionFiles fakeWorkDirRevFile(const QString &diffIndex, const QString &diffIndexCache);
   QVector<Lane> calculateLanes(const CommitInfo &c);
   RevisionFiles parseDiffFormat(const QString &buf, FileNamesLoader &fl, bool cached = false);
   void appendFileName(const QString &name, FileNamesLoader &fl);
   void flushFileNames(FileNamesLoader &fl);
   void setExtStatus(RevisionFiles &rf, const QString &rowSt, int parNum, FileNamesLoader &fl);
   QVector<CommitInfo *>::const_iterator searchCommit(CommitInfo::Field field, const QString &text,
                                                      int startingPoint = 0) const;
   QVector<CommitInfo *>::const_reverse_iterator reverseSearchCommit(CommitInfo::Field field, const QString &text,
                                                                     int startingPoint = 0) const;
   void resetLanes(const CommitInfo &c, bool isFork);
};
