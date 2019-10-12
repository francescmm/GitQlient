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

class QListWidgetItem;
class Git;

namespace Ui
{
class CommitWidget;
}

class CommitWidget : public QWidget
{
   Q_OBJECT

signals:
   void signalChangesCommitted(bool commited);

public:
   explicit CommitWidget(QSharedPointer<Git> git, QWidget *parent = nullptr);

   void init(const QString &shaToAmmend);
   void clear();

private:
   bool mIsAmmend = false;
   Ui::CommitWidget *ui = nullptr;
   QSharedPointer<Git> mGit;

   void addFileToCommitList(QListWidgetItem *item);
   void removeFileFromCommitList(QListWidgetItem *item);
   bool commitChanges();
   bool ammendChanges();
   void contextMenuPopup(const QPoint &);
   void applyChanges();
   QStringList getFiles();
   bool checkMsg(QString &msg);

   static QString lastMsgBeforeError;
};
