#pragma once

/****************************************************************************************
 ** GitQlient is an application to manage and operate one or several Git repositories. With
 ** GitQlient you will be able to add commits, branches and manage all the options Git provides.
 ** Copyright (C) 2021  Francesc Martinez
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

#include <CommitInfo.h>
#include <RevisionFiles.h>
#include <lanes.h>

#include <QHash>
#include <QMutex>
#include <QObject>
#include <QSharedPointer>

struct WipRevisionInfo;

class GitCache : public QObject
{
   Q_OBJECT

signals:
   void signalCacheUpdated();

public:
   struct LocalBranchDistances
   {
      int aheadOrigin = 0;
      int behindOrigin = 0;
   };

   explicit GitCache(QObject *parent = nullptr);
   ~GitCache();

   int commitCount() const;

   CommitInfo commitInfo(const QString &sha);
   CommitInfo commitInfo(int row);

   CommitInfo searchCommitInfo(const QString &text, int startingPoint = 0, bool reverse = false);

   bool isCommitInCurrentGeneologyTree(const QString &sha) const;

   bool insertRevisionFile(const QString &sha1, const QString &sha2, const RevisionFiles &file);
   RevisionFiles revisionFile(const QString &sha1, const QString &sha2) const;

   void clearReferences();
   void insertReference(const QString &sha, References::Type type, const QString &reference);
   bool hasReferences(const QString &sha) const;
   QStringList getReferences(const QString &sha, References::Type type) const;

   void reloadCurrentBranchInfo(const QString &currentBranch, const QString &currentSha);

   bool updateWipCommit(const WipRevisionInfo &wipInfo);

   void setUntrackedFilesList(const QVector<QString> &untrackedFiles);
   bool pendingLocalChanges();

   QVector<QPair<QString, QStringList>> getBranches(References::Type type);
   QHash<QString, QString> getTags(References::Type tagType) const;

   void updateTags(const QHash<QString, QString> &remoteTags);

private:
   friend class GitRepoLoader;

   QMutex mMutex;
   bool mConfigured = true;
   QVector<CommitInfo *> mCommits;
   QHash<QString, CommitInfo> mCommitsMap;
   QHash<QPair<QString, QString>, RevisionFiles> mRevisionFilesMap;
   Lanes mLanes;
   QVector<QString> mUntrackedfiles;
   QHash<QString, References> mReferences;
   QHash<QString, QString> mRemoteTags;

   void setup(const WipRevisionInfo &wipInfo, const QList<CommitInfo> &commits);
   void setConfigurationDone() { mConfigured = true; }

   void insertWipRevision(const WipRevisionInfo &wipInfo);
   RevisionFiles fakeWorkDirRevFile(const QString &diffIndex, const QString &diffIndexCache);
   QVector<Lane> calculateLanes(const CommitInfo &c);
   auto searchCommit(const QString &text, int startingPoint = 0) const;
   auto reverseSearchCommit(const QString &text, int startingPoint = 0) const;
   void resetLanes(const CommitInfo &c, bool isFork);
   bool checkSha(const QString &originalSha, const QString &currentSha) const;
};
