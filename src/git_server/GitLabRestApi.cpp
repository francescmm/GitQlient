#include "GitLabRestApi.h"
#include <GitQlientSettings.h>

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
   mUserId = settings.value(QString("%1/%2-userId").arg(mSettingsKey, mRepoName), "").toString();
   mRepoId = settings.value(QString("%1/%2-repoId").arg(mSettingsKey, mRepoName), "").toString();

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
      const auto tmpDoc = validateData(reply);

      if (tmpDoc.has_value())
      {
         emit signalConnectionSuccessful();
      }
   });
}

void GitLabRestApi::createIssue(const ServerIssue &) { }

void GitLabRestApi::updateIssue(int, const ServerIssue &) { }

void GitLabRestApi::createPullRequest(const ServerPullRequest &) { }

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
   request.setUrl(formatUrl(page));
   request.setRawHeader("User-Agent", "GitQlient v1.2.0");
   request.setRawHeader("X-Custom-User-Agent", "GitQlient v1.2.0");
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
   const auto tmpDoc = validateData(reply);

   if (tmpDoc.has_value())
   {
      const auto doc = tmpDoc.value();
      const auto list = tmpDoc->toVariant().toList();

      if (!list.isEmpty())
      {
         const auto firstUser = list.first().toMap();

         mUserId = firstUser.value("id").toString();

         GitQlientSettings settings;
         settings.setValue(QString("%1/%2-userId").arg(mSettingsKey, mRepoName), mUserId);
      }
   }
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
   const auto tmpDoc = validateData(reply);

   if (tmpDoc.has_value())
   {
      const auto projectsObj = tmpDoc->toVariant().toList();

      for (const auto projObj : projectsObj)
      {
         const auto labelMap = projObj.toMap();

         if (labelMap.value("path").toString() == mRepoName)
         {
            mRepoId = labelMap.value("id").toString();

            GitQlientSettings settings;
            settings.setValue(QString("%1/%2-repoId").arg(mSettingsKey, mRepoName), mRepoId);
            break;
         }
      }
   }
}

void GitLabRestApi::onLabelsReceived()
{
   const auto reply = qobject_cast<QNetworkReply *>(sender());
   const auto tmpDoc = validateData(reply);

   if (tmpDoc.has_value())
   {
      const auto labelsObj = tmpDoc->toVariant().toList();

      QVector<ServerLabel> labels;

      for (const auto labelObj : labelsObj)
      {
         const auto labelMap = labelObj.toMap();
         ServerLabel sLabel { labelMap.value("id").toInt(),
                              "",
                              "",
                              labelMap.value("name").toString(),
                              labelMap.value("description").toString(),
                              labelMap.value("color").toString(),
                              "" };

         labels.append(std::move(sLabel));
      }

      emit signalLabelsReceived(labels);
   }
}

void GitLabRestApi::onMilestonesReceived()
{
   const auto reply = qobject_cast<QNetworkReply *>(sender());
   const auto tmpDoc = validateData(reply);

   if (tmpDoc.has_value())
   {
      const auto milestonesObj = tmpDoc->toVariant().toList();

      QVector<ServerMilestone> milestones;

      for (const auto milestoneObj : milestonesObj)
      {
         const auto labelMap = milestoneObj.toMap();
         ServerMilestone sMilestone { labelMap.value("id").toInt(),
                                      0,
                                      labelMap.value("iid").toString(),
                                      labelMap.value("title").toString(),
                                      labelMap.value("description").toString(),
                                      labelMap.value("state").toString() == "active" };

         milestones.append(std::move(sMilestone));
      }

      emit signalMilestonesReceived(milestones);
   }
}
