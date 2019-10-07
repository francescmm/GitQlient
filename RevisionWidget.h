#pragma once

/****************************************************************************************
 ** GitQlientl is an application to manage and operate one or several Git repositories. With
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

namespace Ui
{
class RevisionWidget;
}

class RevsView;
class RevFile;
class QListWidgetItem;
class Rev;

class RevisionWidget : public QWidget
{
   Q_OBJECT

signals:
   void signalOpenFileCommit(const QString &currentSha, const QString &previousSha, const QString &file);
   void signalOpenFileContextMenu(const QString &, int);

public:
   explicit RevisionWidget(QWidget *parent = nullptr);
   ~RevisionWidget();

   void setup(RevsView *);
   void setCurrentCommitSha(const QString &sha);
   QString getCurrentCommitSha() const;
   void clear();

private:
   Ui::RevisionWidget *ui;
   QString mCurrentSha;
   QString mParentSha;
};
