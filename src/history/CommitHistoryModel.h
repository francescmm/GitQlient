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

#include <QAbstractItemModel>
#include <QSharedPointer>

class RevisionsCache;
class GitBase;
class CommitInfo;
/**
 * @brief
 *
 * @enum CommitHistoryColumns
 */
enum class CommitHistoryColumns;

/**
 * @brief
 *
 * @class CommitHistoryModel CommitHistoryModel.h "CommitHistoryModel.h"
 */
class CommitHistoryModel : public QAbstractItemModel
{
   Q_OBJECT
public:
   /**
    * @brief
    *
    * @fn CommitHistoryModel
    * @param cache
    * @param git
    * @param parent
    */
   explicit CommitHistoryModel(const QSharedPointer<RevisionsCache> &cache, const QSharedPointer<GitBase> &git,
                               QObject *parent = nullptr);
   /**
    * @brief
    *
    * @fn ~CommitHistoryModel
    */
   ~CommitHistoryModel() override;

   /**
    * @brief
    *
    * @fn clear
    */
   void clear();
   /**
    * @brief
    *
    * @fn sha
    * @param row
    * @return QString
    */
   QString sha(int row) const;

   /**
    * @brief
    *
    * @fn data
    * @param index
    * @param role
    * @return QVariant
    */
   QVariant data(const QModelIndex &index, int role) const override;
   /**
    * @brief
    *
    * @fn headerData
    * @param s
    * @param o
    * @param role
    * @return QVariant
    */
   QVariant headerData(int s, Qt::Orientation o, int role = Qt::DisplayRole) const override;
   /**
    * @brief
    *
    * @fn index
    * @param r
    * @param c
    * @param par
    * @return QModelIndex
    */
   QModelIndex index(int r, int c, const QModelIndex &par = QModelIndex()) const override;
   /**
    * @brief
    *
    * @fn parent
    * @param index
    * @return QModelIndex
    */
   QModelIndex parent(const QModelIndex &index) const override;
   /**
    * @brief
    *
    * @fn rowCount
    * @param par
    * @return int
    */
   int rowCount(const QModelIndex &par = QModelIndex()) const override;
   /**
    * @brief
    *
    * @fn hasChildren
    * @param par
    * @return bool
    */
   bool hasChildren(const QModelIndex &par = QModelIndex()) const override;
   /**
    * @brief
    *
    * @fn columnCount
    * @param
    * @return int
    */
   int columnCount(const QModelIndex &) const override { return mColumns.count(); }
   /**
    * @brief
    *
    * @fn onNewRevisions
    * @param totalCommits
    */
   void onNewRevisions(int totalCommits);

private:
   QSharedPointer<RevisionsCache> mCache; /**< TODO: describe */
   QSharedPointer<GitBase> mGit; /**< TODO: describe */

   /**
    * @brief
    *
    * @fn getToolTipData
    * @param r
    * @return QVariant
    */
   QVariant getToolTipData(const CommitInfo &r) const;
   /**
    * @brief
    *
    * @fn getDisplayData
    * @param rev
    * @param column
    * @return QVariant
    */
   QVariant getDisplayData(const CommitInfo &rev, int column) const;

   QMap<CommitHistoryColumns, QString> mColumns; /**< TODO: describe */
   int earlyOutputCnt = 0; /**< TODO: describe */
   int rowCnt = 0; /**< TODO: describe */
   QStringList curFNames; /**< TODO: describe */
   QStringList renamedRevs; /**< TODO: describe */
   QHash<QString, QString> renamedPatches; /**< TODO: describe */
};
