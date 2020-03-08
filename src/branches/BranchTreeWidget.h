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

#include <QTreeWidget>

class GitBase;

class BranchTreeWidget : public QTreeWidget
{
   Q_OBJECT

signals:
   void signalBranchesUpdated();
   void signalBranchCheckedOut();
   void signalSelectCommit(const QString &sha);
   void signalMergeRequired(const QString &currentBranch, const QString &fromBranch);

public:
   explicit BranchTreeWidget(const QSharedPointer<GitBase> &git, QWidget *parent = nullptr);
   void setLocalRepo(const bool isLocal) { mLocal = isLocal; }

private:
   bool mLocal = false;
   QSharedPointer<GitBase> mGit;

   void showBranchesContextMenu(const QPoint &pos);
   void checkoutBranch(QTreeWidgetItem *item);
   void selectCommit(QTreeWidgetItem *item);
};