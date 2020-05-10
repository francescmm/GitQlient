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

#include <QTreeWidgetItem>
#include <GitExecResult.h>

/**
 * @brief The GitQlientTreeWidgetItem class is a custom QTreeWidgetItem class that implements two methods that will
 * receive signals from GitBranches. In order to be able to process the data from git, we need to know that the item is
 * still alive. The normal behaviour of QTreeWidgetItem is to not inherit from QObject, so that needs to be done by
 * creating a derived class from that one and adding an additional inheritance from QObject.
 */
class GitQlientTreeWidgetItem : public QObject, public QTreeWidgetItem
{
public:
   /**
    * @brief GitQlientTreeWidgetItem Default constructor
    * @param parent The parent item.
    */
   GitQlientTreeWidgetItem(QTreeWidgetItem *parent);
   /**
    * @brief distancesToMaster Writes the distance to master.
    * @param result The result of the \ref getDistanceBetweenBranchesAsync method in \ref GitBranches.
    */
   void distancesToMaster(GitExecResult result);
   /**
    * @brief distancesToOrigin Writes the distance to origin.
    * @param result The result of the \ref getDistanceBetweenBranchesAsync method in \ref GitBranches.
    */
   void distancesToOrigin(GitExecResult result);
};
