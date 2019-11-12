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

#include <QObject>
#include <QHash>

class Git;

class RevisionsCache : public QObject
{
   Q_OBJECT

signals:
   void signalCacheUpdated();

public:
   explicit RevisionsCache(QObject *parent = nullptr);
   void configure(int numElementsToStore);
   int count() const { return mCommits.count(); }

   CommitInfo getCommitInfoByRow(int row) const;
   CommitInfo getCommitInfo(const QString &sha) const;
   RevisionFile getRevisionFile(const QString &sha) const { return mRevsFiles.value(sha); }

   void insertCommitInfo(CommitInfo rev);
   void insertRevisionFile(const QString &sha, const RevisionFile &file) { mRevsFiles.insert(sha, file); }

   void clear();
   void clearRevisionFile() { mRevsFiles.clear(); }

   bool containsRevisionFile(const QString &sha) const { return mRevsFiles.contains(sha); }

private:
   QVector<CommitInfo *> mCommits;
   QHash<QString, CommitInfo *> revs;
   QHash<QString, RevisionFile> mRevsFiles;
   Lanes lns;
   bool mCacheLocked = true;

   void updateLanes(CommitInfo &c, Lanes &lns);
};
