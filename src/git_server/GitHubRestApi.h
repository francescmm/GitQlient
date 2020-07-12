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

#include <ServerMilestone.h>
#include <ServerLabel.h>

#include <QObject>
#include <QUrl>

struct ServerAuthentication
{
   QString userName;
   QString userPass;
};

class QJsonDocument;
class QNetworkAccessManager;
class QNetworkReply;
struct ServerIssue;
struct ServerPullRequest;

class GitHubRestApi : public QObject
{
   Q_OBJECT

signals:
   void signalConnectionSuccessful();
   void signalLabelsReceived(const QVector<ServerLabel> &labels);
   void signalMilestonesReceived(const QVector<ServerMilestone> &milestones);
   void signalIssueCreated(const QString &url);
   void signalPullRequestCreated(const QString &url);

public:
   explicit GitHubRestApi(const QString &repoOwner, const QString &repoName, const ServerAuthentication &auth,
                          QObject *parent = nullptr);

   void testConnection();

   void createIssue(const ServerIssue &issue);
   void createPullRequest(const ServerPullRequest &pullRequest);

   void requestLabels();
   void getMilestones();

private:
   QString mServerUrl;
   QString mRepoName;
   QString mRepoOwner;
   ServerAuthentication mAuth;
   QNetworkAccessManager *mManager;

   QUrl formatUrl(const QString endPoint) const;

   void validateData(QNetworkReply *reply);
   void onLabelsReceived(const QJsonDocument &doc);
   void onMilestonesReceived(const QJsonDocument &doc);
   void onIssueCreated(const QJsonDocument &doc);
   void onPullRequestCreated(const QJsonDocument &doc);
};
