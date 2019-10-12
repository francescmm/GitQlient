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

#include <QMenu>

class Git;

struct BranchContextMenuConfig
{
   QString currentBranch;
   QString branchSelected;
   bool isLocal;
   QSharedPointer<Git> mGit;
};

class BranchContextMenu : public QMenu
{
   Q_OBJECT

signals:
   void signalBranchesUpdated();
   void signalCheckoutBranch();

public:
   explicit BranchContextMenu(const BranchContextMenuConfig &config, QWidget *parent = nullptr);

private:
   BranchContextMenuConfig mConfig;

   void pull();
   void fetch();
   void push();
   void pushForce();
   void createBranch();
   void createCheckoutBranch();
   void merge();
   void rename();
   void deleteBranch();
};
