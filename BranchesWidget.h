#pragma once

/****************************************************************************************
 ** GitQlient is an application to manage and operate one or several Git repositories. With
 ** GitQlient you will be able to add commits, branches and manage all the options Git provides.
 ** Copyright (C) 2019  Francesc Martinez
 **
 ** LinkedIn: www.linkedin.com/in/cescmm/
 ** Web: www.francescmm.com
 **
 ** This library is free software; you can redistribute it and/or
 ** modify it under the terms of the GNU Lesser General Public
 ** License as published by the Free Software Foundation; either
 ** version 3 of the License, or (at your option) any later version.
 **
 ** This library is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 ** Lesser General Public License for more details.
 **
 ** You should have received a copy of the GNU Lesser General Public
 ** License along with this library; if not, write to the Free Software
 ** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 ***************************************************************************************/

#include <QWidget>

class BranchTreeWidget;
class QListWidget;
class QListWidgetItem;
class QLabel;
class Git;

class BranchesWidget : public QWidget
{
   Q_OBJECT

signals:
   void signalBranchesUpdated();
   void signalSelectCommit(const QString &sha);
   void signalOpenSubmodule(const QString &submoduleName);

public:
   explicit BranchesWidget(QSharedPointer<Git> git, QWidget *parent = nullptr);
   void showBranches();
   void clear();

private:
   QSharedPointer<Git> mGit;
   BranchTreeWidget *mLocalBranchesTree = nullptr;
   BranchTreeWidget *mRemoteBranchesTree = nullptr;
   QListWidget *mTagsList = nullptr;
   QListWidget *mStashesList = nullptr;
   QListWidget *mSubmodulesList = nullptr;
   QLabel *mTagsCount = nullptr;
   QLabel *mTagArrow = nullptr;
   QLabel *mStashesCount = nullptr;
   QLabel *mStashesArrow = nullptr;
   QLabel *mSubmodulesCount = nullptr;
   QLabel *mSubmodulesArrow = nullptr;

   void processLocalBranch(QString branch);
   void processRemoteBranch(QString branch);
   void processTags();
   void processStashes();
   void processSubmodules();
   void adjustBranchesTree(BranchTreeWidget *treeWidget);
   void showTagsContextMenu(const QPoint &p);
   void showStashesContextMenu(const QPoint &p);
   void showSubmodulesContextMenu(const QPoint &p);
   void onTagsHeaderClicked();
   void onStashesHeaderClicked();
   void onSubmodulesHeaderClicked();
   void onTagClicked(QListWidgetItem *item);
   void onStashClicked(QListWidgetItem *item);
};
