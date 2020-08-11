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
#include <QMap>

#include <Issue.h>

class QVBoxLayout;
class QScrollArea;
class GitBase;
class QNetworkAccessManager;
class QLabel;
class QHBoxLayout;

namespace GitServer
{
class IRestApi;
struct Issue;
}

class IssueDetailedView : public QFrame
{
   Q_OBJECT
signals:

public:
   enum class Config
   {
      Issues,
      PullRequests
   };
   explicit IssueDetailedView(const QSharedPointer<GitBase> &git, Config config, QWidget *parent = nullptr);

   void loadData(const GitServer::Issue &issue);

private:
   GitServer::Issue mIssue;
   bool mLoaded = false;
   QSharedPointer<GitBase> mGit;
   GitServer::IRestApi *mApi = nullptr;
   Config mConfig;
   QNetworkAccessManager *mManager;
   QVBoxLayout *mIssuesLayout = nullptr;
   QFrame *mIssuesFrame = nullptr;
   QFrame *mIssueDetailedView = nullptr;
   QHBoxLayout *mIssueDetailedViewLayout = nullptr;
   QScrollArea *mScrollArea = nullptr;

   void storeCreatorAvatar(QLabel *avatar, const QString &fileName);
   void onCommentReceived(const GitServer::Issue &issue);
};
