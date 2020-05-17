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

#include <QListWidget>

class UntrackedFilesList : public QListWidget
{
   Q_OBJECT

signals:
   void signalStageFile(QListWidgetItem *item);
   void signalCheckoutPerformed();
   void signalShowDiff(const QString &fileName);

public:
   explicit UntrackedFilesList(QWidget *parent = nullptr);
   void setWorkingDirectory(const QString &workingDir) { mWorkingDir = workingDir; }

private:
   QString mWorkingDir;
   QListWidgetItem *mSelectedItem = nullptr;

   void onContextMenu(const QPoint &pos);
   void onStageFile();
   void onDeleteFile();
   void onDoubleClick(QListWidgetItem *item);
};
