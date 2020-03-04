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

#include <QAbstractItemModel>
#include <QSharedPointer>

class RevisionsCache;
class GitBase;
class CommitInfo;
enum class CommitHistoryColumns;

class CommitHistoryModel : public QAbstractItemModel
{
   Q_OBJECT
public:
   explicit CommitHistoryModel(const QSharedPointer<RevisionsCache> &cache, const QSharedPointer<GitBase> &git,
                               QObject *parent = nullptr);
   ~CommitHistoryModel() override;

   void clear();
   QString sha(int row) const;

   QVariant data(const QModelIndex &index, int role) const override;
   QVariant headerData(int s, Qt::Orientation o, int role = Qt::DisplayRole) const override;
   QModelIndex index(int r, int c, const QModelIndex &par = QModelIndex()) const override;
   QModelIndex parent(const QModelIndex &index) const override;
   int rowCount(const QModelIndex &par = QModelIndex()) const override;
   bool hasChildren(const QModelIndex &par = QModelIndex()) const override;
   int columnCount(const QModelIndex &) const override { return mColumns.count(); }
   int columnCount() const { return mColumns.count(); }
   void onNewRevisions(int totalCommits);

private:
   QSharedPointer<RevisionsCache> mCache;
   QSharedPointer<GitBase> mGit;

   QVariant getToolTipData(const CommitInfo &r) const;
   QVariant getDisplayData(const CommitInfo &rev, int column) const;

   QMap<CommitHistoryColumns, QString> mColumns;
   int earlyOutputCnt = 0;
   int rowCnt = 0;
   QStringList curFNames;
   QStringList renamedRevs;
   QHash<QString, QString> renamedPatches;
};
