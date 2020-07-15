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
#include <ServerPullRequest.h>

#include <QObject>
#include <QMap>
#include <QNetworkRequest>

class QNetworkAccessManager;
class QNetworkReply;
struct ServerIssue;

struct ServerAuthentication
{
   QString userName;
   QString userPass;
   QString endpointUrl;
};

class IRestApi : public QObject
{
   Q_OBJECT

signals:
   void signalConnectionSuccessful();
   void signalLabelsReceived(const QVector<ServerLabel> &labels);
   void signalMilestonesReceived(const QVector<ServerMilestone> &milestones);
   void signalIssueCreated(QString url);
   void signalIssueUpdated();
   void signalPullRequestCreated(QString url);
   void signalPullRequestsReceived(QMap<QString, ServerPullRequest> prs);

public:
   explicit IRestApi(const ServerAuthentication &auth, QObject *parent = nullptr);
   virtual ~IRestApi() = default;

   static std::optional<QJsonDocument> validateData(QNetworkReply *reply);

   virtual void testConnection() = 0;
   virtual void createIssue(const ServerIssue &issue) = 0;
   virtual void updateIssue(int issueNumber, const ServerIssue &issue) = 0;
   virtual void createPullRequest(const ServerPullRequest &pullRequest) = 0;
   virtual void requestLabels() = 0;
   virtual void requestMilestones() = 0;
   virtual void requestPullRequestsState() = 0;

protected:
   QNetworkAccessManager *mManager = nullptr;
   ServerAuthentication mAuth;

   virtual QUrl formatUrl(const QString page) const final;
   virtual QNetworkRequest createRequest(const QString &page) const = 0;
};
