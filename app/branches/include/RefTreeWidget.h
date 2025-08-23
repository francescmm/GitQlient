#pragma once

/****************************************************************************************
 ** GitQlient is an application to manage and operate one or several Git repositories. With
 ** GitQlient you will be able to add commits, branches and manage all the options Git provides.
 ** Copyright (C) 2021  Francesc Martinez
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

class RefTreeWidget : public QTreeWidget
{
   Q_OBJECT

public:
   /**
    * @brief Default constructor
    * @param cache The GitQlient cache.
    * @param git The git object to perform Git operations.
    * @param parentThe parent widget if needed.
    */
   explicit RefTreeWidget(QWidget *parent = nullptr);
   /**
    * @brief focusOnBranch Sets the focus of the three in the item specified in  @p branch starting from the position @p
    * lastPos.
    * @param item The text to search in the tree and set the focus.
    * @param lastPos Starting position for the search.
    * @return
    */
   int focusOnBranch(const QString &itemText, int startSearchPos = -1);

protected:
   QVector<QTreeWidgetItem *> findChildItem(const QString &text) const;
};
