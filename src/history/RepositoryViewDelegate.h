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

#include <QStyledItemDelegate>

class CommitHistoryView;
class RevisionsCache;
class GitBase;

const int ROW_HEIGHT = 25;
const int LANE_WIDTH = 3 * ROW_HEIGHT / 4;

enum class LaneType;

/**
 * @brief The RepositoryViewDelegate class is the delegate overloads the paint functionality in the RepositoryView. This
 * class is the responsible of painting the graph of the repository. In addition to paint all the columns, implements
 * special functionality to paint the tags, the branch names and how they are represented (local or remote)
 *
 */
class RepositoryViewDelegate : public QStyledItemDelegate
{
   Q_OBJECT

public:
   /**
    * @brief Default constructor.
    *
    * @param cache The cache for the current repository.
    * @param git The git object to execute git commands.
    * @param view The view that uses the delegate.
    */
   RepositoryViewDelegate(const QSharedPointer<RevisionsCache> &cache, const QSharedPointer<GitBase> &git,
                          CommitHistoryView *view);

   /**
    * @brief Overrided method to paint the different columns and rows in the view.
    *
    * @param p The painter device.
    * @param o The style options of the item.
    * @param i The index with the item data.
    */
   void paint(QPainter *p, const QStyleOptionViewItem &o, const QModelIndex &i) const override;
   /**
    * @brief The size hint returns the width and height for rendering purposes.
    *
    * @return QSize returns the size of a row.
    */
   QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override;

private:
   QSharedPointer<RevisionsCache> mCache;
   QSharedPointer<GitBase> mGit;
   CommitHistoryView *mView = nullptr;
   int diffTargetRow = -1;

   /**
    * @brief Paints the log column. This method is in charge of painting the commit message as well as tags or branches.
    *
    * @param p The painter device.
    * @param o The style options of the item.
    * @param i The index with the item data.
    */
   void paintLog(QPainter *p, const QStyleOptionViewItem &o, const QModelIndex &i) const;
   /**
    * @brief Method that sets up the configuration to paint the lane for the commit graph representation.
    *
    * @param p The painter device.
    * @param o The style options of the item.
    * @param index The index with the item data.
    */
   void paintGraph(QPainter *p, const QStyleOptionViewItem &o, const QModelIndex &index) const;

   /**
    * @brief Specialization method called by @ref paintGrapth that does the actual lane painting.
    *
    * @param p The painter device.
    * @param type The type of lane to paint.
    * @param laneHeadPresent Tells the method if the lane contains a head.
    * @param x1 X coordinate where the painting starts
    * @param x2 X coordinate where the painting ends
    * @param col Color of the lane
    * @param activeCol Color of the active lane
    * @param mergeColor Color of the lane where the merge comes from in case the commit is a end-merge point.
    * @param isWip Tells the method if it's the WIP commit so it's painted differently.
    */
   void paintGraphLane(QPainter *p, const LaneType type, bool laneHeadPresent, int x1, int x2, const QColor &col,
                       const QColor &activeCol, const QColor &mergeColor, bool isWip = false) const;

   /**
    * @brief Specialized method that paints a tag in the commit message column.
    *
    * @param painter The painter device.
    * @param opt The style options of the item.
    * @param startPoint The starting X coordinate for the tag.
    * @param sha The SHA reference to paint. It can be local branch, remote branch, tag or it could be detached.
    */
   void paintTagBranch(QPainter *painter, QStyleOptionViewItem opt, int &startPoint, const QString &sha) const;

   QColor getMergeColor(LaneType ln, const QVector<LaneType> &lanes, int currentLane, const QColor &defaultColor,
                        bool &isSet) const;
};
