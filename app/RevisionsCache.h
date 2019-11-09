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
#include <Revision.h>

#include <QObject>
#include <QHash>
#include <QSharedPointer>

class Git;

class RevisionsCache : public QObject
{
   Q_OBJECT

signals:
   void signalCacheUpdated();

public:
   explicit RevisionsCache(QObject *parent = nullptr);

   QString sha(int row) const;
   Revision getRevLookupByRow(int row) const;
   Revision getRevLookup(const QString &sha) const;
   void insertRevision(const Revision &rev);
   void updateLanes(Revision &c, Lanes &lns);
   QString getLaneParent(const QString &fromSHA, int laneNum);
   QString getShortLog(const QString &sha) const;
   int row(const QString &sha) const;
   int count() const { return revOrder.count(); }
   bool isEmpty() const { return revOrder.isEmpty(); }
   void clear();
   QString &createRevisionSha(int index) { return revOrder[index]; }
   QString getRevisionSha(int index) const { return revOrder.at(index); }
   int revOrderCount() const { return revOrder.count(); }
   bool contains(const QString &sha) { return revs.contains(sha); }
   RevisionFile getRevisionFile(const QString &sha) const { return mRevsFiles.value(sha); }
   void insertRevisionFile(const QString &sha, const RevisionFile &file) { mRevsFiles.insert(sha, file); }
   void removeRevisionFile(const QString &sha) { mRevsFiles.remove(sha); }
   void clearRevisionFile() { mRevsFiles.clear(); }
   bool containsRevisionFile(const QString &sha) const { return mRevsFiles.contains(sha); }

private:
   QHash<QString, Revision> revs;
   QHash<QString, RevisionFile> mRevsFiles;
   QVector<QString> revOrder;
   Lanes lns;
};
