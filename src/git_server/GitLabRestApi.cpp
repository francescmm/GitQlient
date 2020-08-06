#include "GitLabRestApi.h"
#include <GitQlientSettings.h>
#include <ServerIssue.h>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QTimer>

GitLabRestApi::GitLabRestApi(const QString &userName, const QString &repoName, const QString &settingsKey,
                             const ServerAuthentication &auth, QObject *parent)
   : IRestApi(auth, parent)
   , mUserName(userName)
   , mRepoName(repoName)
   , mSettingsKey(settingsKey)
{
   GitQlientSettings settings;
   mUserId = settings.globalValue(QString("%1/%2-userId").arg(mSettingsKey, mRepoName), "").toString();
   mRepoId = settings.globalValue(QString("%1/%2-repoId").arg(mSettingsKey, mRepoName), "").toString();

   if (mRepoId.isEmpty())
      getProjects();

   if (mUserId.isEmpty())
      getUserInfo();
}

void GitLabRestApi::testConnection()
{
   auto request = createRequest("/users?username=%1");
   auto url = request.url();

   QUrlQuery query;
   query.addQueryItem("username", mUserName);
   url.setQuery(query);
   request.setUrl(url);

   const auto reply = mManager->get(request);

   connect(reply, &QNetworkReply::finished, this, [this]() {
      const auto reply = qobject_cast<QNetworkReply *>(sender());
      QString errorStr;
      const auto tmpDoc = validateData(reply, errorStr);

      if (!tmpDoc.isEmpty())
         emit connectionTested();
      else
         emit errorOccurred(errorStr);
   });
}

void GitLabRestApi::createIssue(const ServerIssue &issue)
{
   auto request = createRequest(QString("/projects/%1/issues").arg(mRepoId));
   auto url = request.url();

   QUrlQuery query;
   query.addQueryItem("title", issue.title);
   query.addQueryItem("description", QString::fromUtf8(issue.body));

   if (!issue.assignees.isEmpty())
      query.addQueryItem("assignee_ids", mUserId);

   if (issue.milestone != -1)
      query.addQueryItem("milestone_id", QString::number(issue.milestone));

   if (!issue.labels.isEmpty())
      query.addQueryItem("labels", issue.labels.join(","));

   url.setQuery(query);
   request.setUrl(url);

   const auto reply = mManager->post(request, "");

   connect(reply, &QNetworkReply::finished, this, &GitLabRestApi::onIssueCreated);
}

void GitLabRestApi::updateIssue(int, const ServerIssue &) { }

void GitLabRestApi::createPullRequest(const ServerPullRequest &pr)
{
   auto request = createRequest(QString("/projects/%1/merge_requests").arg(mRepoId));
   auto url = request.url();

   QUrlQuery query;
   query.addQueryItem("title", pr.title);
   query.addQueryItem("description", QString::fromUtf8(pr.body));
   query.addQueryItem("assignee_ids", mUserId);
   query.addQueryItem("target_branch", pr.base);
   query.addQueryItem("source_branch", pr.head);
   query.addQueryItem("allow_collaboration", QVariant(pr.maintainerCanModify).toString());

   if (pr.milestone != -1)
      query.addQueryItem("milestone_id", QString::number(pr.milestone));

   if (!pr.labels.isEmpty())
      query.addQueryItem("labels", pr.labels.join(","));

   url.setQuery(query);
   request.setUrl(url);

   const auto reply = mManager->post(request, "");

   connect(reply, &QNetworkReply::finished, this, &GitLabRestApi::onMergeRequestCreated);
}

void GitLabRestApi::requestLabels()
{
   const auto reply = mManager->get(createRequest(QString("/projects/%1/labels").arg(mRepoId)));

   connect(reply, &QNetworkReply::finished, this, &GitLabRestApi::onLabelsReceived);
}

void GitLabRestApi::requestMilestones()
{
   const auto reply = mManager->get(createRequest(QString("/projects/%1/milestones").arg(mRepoId)));

   connect(reply, &QNetworkReply::finished, this, &GitLabRestApi::onMilestonesReceived);
}

void GitLabRestApi::requestPullRequestsState() { }

QNetworkRequest GitLabRestApi::createRequest(const QString &page) const
{
   QNetworkRequest request;
   request.setUrl(QString(mAuth.endpointUrl + page));
   request.setRawHeader("User-Agent", "GitQlient");
   request.setRawHeader("X-Custom-User-Agent", "GitQlient");
   request.setRawHeader("Content-Type", "application/json");
   request.setRawHeader(QByteArray("PRIVATE-TOKEN"),
                        QByteArray(QString(QStringLiteral("%1")).arg(mAuth.userPass).toLocal8Bit()));

   return request;
}

void GitLabRestApi::getUserInfo() const
{
   auto request = createRequest("/users");
   auto url = request.url();

   QUrlQuery query;
   query.addQueryItem("username", mUserName);
   url.setQuery(query);
   request.setUrl(url);

   const auto reply = mManager->get(request);

   connect(reply, &QNetworkReply::finished, this, &GitLabRestApi::onUserInfoReceived);
}

void GitLabRestApi::onUserInfoReceived()
{
   const auto reply = qobject_cast<QNetworkReply *>(sender());
   QString errorStr;
   const auto tmpDoc = validateData(reply, errorStr);

   if (!tmpDoc.isEmpty())
   {
      const auto doc = tmpDoc;
      const auto list = tmpDoc.toVariant().toList();

      if (!list.isEmpty())
      {
         const auto firstUser = list.first().toMap();

         mUserId = firstUser.value("id").toString();

         GitQlientSettings settings;
         settings.setGlobalValue(QString("%1/%2-userId").arg(mSettingsKey, mRepoName), mUserId);
      }
   }
   else
      emit errorOccurred(errorStr);
}

void GitLabRestApi::getProjects()
{
   auto request = createRequest(QString("/users/%1/projects").arg(mUserName));
   const auto reply = mManager->get(request);

   connect(reply, &QNetworkReply::finished, this, &GitLabRestApi::onProjectsReceived);
}

void GitLabRestApi::onProjectsReceived()
{
   const auto reply = qobject_cast<QNetworkReply *>(sender());
   QString errorStr;
   const auto tmpDoc = validateData(reply, errorStr);

   if (!tmpDoc.isEmpty())
   {
      const auto projectsObj = tmpDoc.toVariant().toList();

      for (const auto projObj : projectsObj)
      {
         const auto labelMap = projObj.toMap();

         if (labelMap.value("path").toString() == mRepoName)
         {
            mRepoId = labelMap.value("id").toString();

            GitQlientSettings settings;
            settings.setGlobalValue(QString("%1/%2-repoId").arg(mSettingsKey, mRepoName), mRepoId);
            break;
         }
      }
   }
   else
      emit errorOccurred(errorStr);
}

void GitLabRestApi::onLabelsReceived()
{
   const auto reply = qobject_cast<QNetworkReply *>(sender());
   QString errorStr;
   const auto tmpDoc = validateData(reply, errorStr);

   if (!tmpDoc.isEmpty())
   {
      const auto labelsObj = tmpDoc.toVariant().toList();

      QVector<ServerLabel> labels;

      for (const auto labelObj : labelsObj)
      {
         const auto labelMap = labelObj.toMap();
         ServerLabel sLabel { labelMap.value("id").toString().toInt(),
                              "",
                              "",
                              labelMap.value("name").toString(),
                              labelMap.value("description").toString(),
                              labelMap.value("color").toString(),
                              "" };

         labels.append(std::move(sLabel));
      }

      emit labelsReceived(labels);
   }
   else
      emit errorOccurred(errorStr);
}

void GitLabRestApi::onMilestonesReceived()
{
   const auto reply = qobject_cast<QNetworkReply *>(sender());
   QString errorStr;
   const auto tmpDoc = validateData(reply, errorStr);

   if (!tmpDoc.isEmpty())
   {
      const auto milestonesObj = tmpDoc.toVariant().toList();

      QVector<ServerMilestone> milestones;

      for (const auto milestoneObj : milestonesObj)
      {
         const auto labelMap = milestoneObj.toMap();
         ServerMilestone sMilestone {
            labelMap.value("id").toString().toInt(),  labelMap.value("id").toString().toInt(),
            labelMap.value("iid").toString(),         labelMap.value("title").toString(),
            labelMap.value("description").toString(), labelMap.value("state").toString() == "active"
         };

         milestones.append(std::move(sMilestone));
      }

      emit milestonesReceived(milestones);
   }
   else
      emit errorOccurred(errorStr);
}

void GitLabRestApi::onIssueCreated()
{
   const auto reply = qobject_cast<QNetworkReply *>(sender());
   QString errorStr;
   const auto tmpDoc = validateData(reply, errorStr);

   if (!tmpDoc.isEmpty())
   {
      const auto doc = tmpDoc;
      const auto issue = doc.object();
      const auto list = tmpDoc.toVariant().toList();
      const auto url = issue[QStringLiteral("web_url")].toString();

      emit issueCreated(url);
   }
   else
      emit errorOccurred(errorStr);
}

void GitLabRestApi::onMergeRequestCreated()
{
   const auto reply = qobject_cast<QNetworkReply *>(sender());
   QString errorStr;
   const auto tmpDoc = validateData(reply, errorStr);

   if (!tmpDoc.isEmpty())
   {
      const auto doc = tmpDoc;
      const auto issue = doc.object();
      const auto list = tmpDoc.toVariant().toList();
      const auto url = issue[QStringLiteral("web_url")].toString();

      emit pullRequestCreated(url);
   }
   else
      emit errorOccurred(errorStr);
}
