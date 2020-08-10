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

#include <QFrame>

class QVBoxLayout;
class QScrollArea;
class GitBase;
class QLabel;

namespace GitServer
{
class IRestApi;
struct Issue;
}

class IssuesWidget : public QFrame
{
   Q_OBJECT
signals:
   void selected(const GitServer::Issue &issue);

public:
   enum class Config
   {
      Issues,
      PullRequests
   };
   explicit IssuesWidget(const QSharedPointer<GitBase> &git, Config config, QWidget *parent = nullptr);

   void loadData();

private:
   QSharedPointer<GitBase> mGit;
   GitServer::IRestApi *mApi = nullptr;
   Config mConfig;
   QVBoxLayout *mIssuesLayout = nullptr;
   QFrame *mIssuesWidget = nullptr;
   QScrollArea *mScrollArea = nullptr;
   QLabel *mArrow = nullptr;

   void onIssuesReceived(const QVector<GitServer::Issue> &issues);
   void onHeaderClicked();
};
