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

#include <IRestApi.h>

#include <QUrl>
#include <QNetworkRequest>

class QJsonDocument;
class QNetworkReply;
struct ServerIssue;

class GitHubRestApi final : public IRestApi
{
   Q_OBJECT

public:
   explicit GitHubRestApi(QString repoOwner, QString repoName, const ServerAuthentication &auth,
                          QObject *parent = nullptr);

   void testConnection() override;
   void createIssue(const ServerIssue &issue) override;
   void updateIssue(int issueNumber, const ServerIssue &issue) override;
   void createPullRequest(const ServerPullRequest &pullRequest) override;
   void requestLabels() override;
   void requestMilestones() override;
   void requestPullRequestsState() override;
   void mergePullRequest(int number, const QByteArray &data) override;

private:
   QMap<QString, ServerPullRequest> mPulls;
   QString mRepoEndpoint;
   int mPrRequested = 0;

   QNetworkRequest createRequest(const QString &page) const override;
   void onLabelsReceived();
   void onMilestonesReceived();
   void onIssueCreated();
   void onPullRequestCreated();
   void processPullRequets();
   void onPullRequestStatusReceived();
   void onPullRequestMerged();
};
