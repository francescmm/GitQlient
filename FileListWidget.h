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

#include <QListWidget>

class Domain;
class StateInfo;
class Git;
class RevFile;

class FileListWidget : public QListWidget
{
   Q_OBJECT

signals:
   void contextMenu(const QString &, int);

public:
   explicit FileListWidget(QSharedPointer<Git> git, QWidget *parent = nullptr);
   void setup(Domain *dm);
   void update(const RevFile *files, bool newFiles);
   void addItem(const QString &label, const QColor &clr);

private:
   void showContextMenu(const QPoint &);
   void insertFiles(const RevFile *files);

   QSharedPointer<Git> mGit = nullptr;
   Domain *d = nullptr;
   StateInfo *st = nullptr;
};
