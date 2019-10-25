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

#include <QWidget>
#include <QSet>

class QTabWidget;
class GitQlientRepo;

class GitQlient : public QWidget
{
   Q_OBJECT
public:
   explicit GitQlient(QWidget *parent = nullptr);

   void setRepositories(const QStringList repositories);

private:
   bool mFirstRepoInitialized = false;
   QTabWidget *mRepos = nullptr;
   QSet<QString> mCurrentRepos;

   void setRepoName(const QString &repoName);
   void openRepo();
   void addRepoTab(const QString &repoPath = "");
   void repoClosed(int tabIndex);
   void closeTab(int tabIndex);
};
