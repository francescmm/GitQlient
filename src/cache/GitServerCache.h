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

#include <QObject>
#include <QMap>

#include <PullRequest.h>
#include <Issue.h>
#include <Platform.h>

namespace GitServer
{
class IRestApi;
struct PullRequest;
struct Issue;
struct Label;
struct Milestone;
}

class GitServerCache : public QObject
{
   Q_OBJECT

public:
   explicit GitServerCache(QObject *parent = nullptr);
   ~GitServerCache();

   bool init(const QString &serverUrl, const QPair<QString, QString> &repoInfo);

   QVector<GitServer::PullRequest> getPullRequests() const { return mPullRequests.values().toVector(); }
   GitServer::PullRequest getPullRequest(int number) const { return mPullRequests.value(number); }
   GitServer::PullRequest getPullRequest(const QString &sha) const;
   QVector<GitServer::Issue> getIssues() const { return mIssues.values().toVector(); }
   GitServer::Issue getIssue(int number) const { return mIssues.value(number); }
   QVector<GitServer::Label> getLabels() const { return mLabels; }
   QVector<GitServer::Milestone> getMilestones() const { return mMilestones; }

   GitServer::Platform getPlatform() const;

private:
   bool mInit = false;
   QScopedPointer<GitServer::IRestApi> mApi;
   QMap<int, GitServer::PullRequest> mPullRequests;
   QMap<int, GitServer::Issue> mIssues;
   QVector<GitServer::Label> mLabels;
   QVector<GitServer::Milestone> mMilestones;

   void initLabels(const QVector<GitServer::Label> &labels);
   void initMilestones(const QVector<GitServer::Milestone> &milestones);
   void initIssues(const QVector<GitServer::Issue> &issues);
   void initPullRequests(const QVector<GitServer::PullRequest> &prs);
};
