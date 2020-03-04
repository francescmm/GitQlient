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

class RepositoryViewDelegate : public QStyledItemDelegate
{
   Q_OBJECT

public:
   RepositoryViewDelegate(const QSharedPointer<RevisionsCache> &cache, const QSharedPointer<GitBase> &git,
                          CommitHistoryView *view);

   void paint(QPainter *p, const QStyleOptionViewItem &o, const QModelIndex &i) const override;
   QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override;

private:
   QSharedPointer<RevisionsCache> mCache;
   QSharedPointer<GitBase> mGit;
   CommitHistoryView *mView = nullptr;
   int diffTargetRow = -1;

   void paintLog(QPainter *p, const QStyleOptionViewItem &o, const QModelIndex &i) const;
   void paintGraph(QPainter *p, const QStyleOptionViewItem &o, const QModelIndex &index) const;
   void paintGraphLane(QPainter *p, const LaneType type, bool laneHeadPresent, int x1, int x2, const QColor &col,
                       const QColor &activeCol, const QColor &mergeColor, bool isWip = false) const;
   void paintTagBranch(QPainter *painter, QStyleOptionViewItem opt, int &startPoint, const QString &sha) const;
};
