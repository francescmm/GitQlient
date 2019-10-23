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

#include <QObject>
#include <QHash>
#include <QVector>
#include <QSharedPointer>

class Git;
class Revision;

class RevisionsCache : public QObject
{
   Q_OBJECT

signals:
   void signalCacheUpdated();

public:
   explicit RevisionsCache(QSharedPointer<Git> git, QObject *parent = nullptr);

   QString sha(int row) const;
   const Revision *revLookup(int row) const;
   const Revision *revLookup(const QString &sha) const;
   void insertRevision(const QString sha, const Revision &rev);
   QString getShortLog(const QString &sha) const;
   int row(const QString &sha) const;
   int count() const { return revOrder.count(); }
   bool isEmpty() const { return revOrder.isEmpty(); }
   void flushTail(int earlyOutputCnt, int earlyOutputCntBase);
   void clear();
   QString &createRevisionSha(int index) { return revOrder[index]; }
   QString getRevisionSha(int index) const { return revOrder.at(index); }
   int revOrderCount() const { return revOrder.count(); }
   bool contains(const QString &sha) { return revs.contains(sha); }

   static const int MAX_DICT_SIZE = 100003; // must be a prime number see QDict docs

private:
   QSharedPointer<Git> mGit;
   QHash<QString, const Revision *> revs;
   QVector<QString> revOrder;
};
