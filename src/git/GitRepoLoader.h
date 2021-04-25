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
#include <GitExecResult.h>

#include <QObject>
#include <QSharedPointer>
#include <QVector>

class GitBase;
class GitCache;
struct WipRevisionInfo;
class GitQlientSettings;

class GitRepoLoader : public QObject
{
   Q_OBJECT

signals:
   void signalLoadingStarted();
   void signalLoadingFinished(bool full);
   void cancelAllProcesses(QPrivateSignal);

public slots:
   bool load(bool refreshReferences);

public:
   explicit GitRepoLoader(QSharedPointer<GitBase> gitBase, QSharedPointer<GitCache> cache,
                          const QSharedPointer<GitQlientSettings> &settings, QObject *parent = nullptr);
   void cancelAll();
   void setShowAll(bool showAll = true) { mShowAll = showAll; }
   bool load();

private:
   bool mShowAll = true;
   bool mLocked = false;
   bool mRefreshReferences = true;
   QSharedPointer<GitBase> mGitBase;
   QSharedPointer<GitCache> mRevCache;
   QSharedPointer<GitQlientSettings> mSettings;

   bool configureRepoDirectory();
   void loadReferences();
   void requestRevisions();
   void processRevision(QByteArray ba);
   QVector<CommitInfo> processUnsignedLog(QByteArray &log) const;
   QVector<CommitInfo> processSignedLog(QByteArray &log) const;
   CommitInfo parseCommitData(QByteArray &commitData) const;
};
