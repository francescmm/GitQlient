#include "GitHubRestApi.h"
#include <ServerIssue.h>

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QLogger.h>

using namespace QLogger;

GitHubRestApi::GitHubRestApi(const QString &repoOwner, const QString &repoName, const ServerAuthentication &auth,
                             QObject *parent)
   : QObject(parent)
   , mEndpointUrl(auth.endpointUrl)
{
   mManager = new QNetworkAccessManager();
   connect(mManager, &QNetworkAccessManager::finished, this, &GitHubRestApi::validateData);

   if (!mEndpointUrl.endsWith("/"))
      mEndpointUrl.append("/");

   mRepoOwner = repoOwner;

   if (!mRepoOwner.endsWith("/"))
      mRepoOwner.append("/");

   mRepoName = repoName;

   if (!mRepoName.endsWith("/"))
      mRepoName.append("/");

   mAuth = auth;
}

void GitHubRestApi::testConnection()
{
   auto request = createRequest("");
   request.setUrl(mEndpointUrl);

   mManager->get(request);
}

void GitHubRestApi::createIssue(const ServerIssue &issue)
{
   QJsonDocument doc(issue.toJson());
   const auto data = doc.toJson(QJsonDocument::Compact);

   auto request = createRequest("issues");
   request.setRawHeader("Content-Length", QByteArray::number(data.size()));
   mManager->post(request, data);
}

void GitHubRestApi::updateIssue(int issueNumber, const ServerIssue &issue)
{
   QJsonDocument doc(issue.toJson());
   const auto data = doc.toJson(QJsonDocument::Compact);

   auto request = createRequest(QString("issues/%1").arg(issueNumber));
   request.setRawHeader("Content-Length", QByteArray::number(data.size()));
   mManager->post(request, data);
}

void GitHubRestApi::createPullRequest(const ServerPullRequest &pullRequest)
{
   QJsonDocument doc(pullRequest.toJson());
   const auto data = doc.toJson(QJsonDocument::Compact);

   auto request = createRequest("pulls");
   request.setRawHeader("Content-Length", QByteArray::number(data.size()));

   mManager->post(request, data);
}

void GitHubRestApi::requestLabels()
{
   mManager->get(createRequest("labels"));
}

void GitHubRestApi::getMilestones()
{
   mManager->get(createRequest("milestones"));
}

void GitHubRestApi::requestPullRequestsState()
{
   mManager->get(createRequest("pulls"));
}

QUrl GitHubRestApi::formatUrl(const QString page) const
{
   auto tmpUrl = mEndpointUrl + "repos/" + mRepoOwner + mRepoName + page;

   if (tmpUrl.endsWith("/"))
      tmpUrl = tmpUrl.left(tmpUrl.size() - 1);

   return QUrl(tmpUrl);
}

QNetworkRequest GitHubRestApi::createRequest(const QString &page) const
{
   QNetworkRequest request;
   request.setUrl(formatUrl(page));
   request.setRawHeader("User-Agent", "GitQlient v1.2.0");
   request.setRawHeader("X-Custom-User-Agent", "GitQlient v1.2.0");
   request.setRawHeader("Content-Type", "application/json");
   request.setRawHeader("Accept", "application/vnd.github.v3+json");
   request.setRawHeader(
       QByteArray("Authorization"),
       QByteArray("Basic ")
           + QByteArray(QString(QStringLiteral("%1:%2")).arg(mAuth.userName).arg(mAuth.userPass).toLocal8Bit())
                 .toBase64());

   return request;
}

void GitHubRestApi::onLabelsReceived(const QJsonDocument &doc)
{
   QVector<ServerLabel> labels;
   const auto labelsArray = doc.array();

   for (auto label : labelsArray)
   {
      const auto jobObject = label.toObject();
      ServerLabel sLabel { jobObject[QStringLiteral("id")].toInt(),
                           jobObject[QStringLiteral("node_id")].toString(),
                           jobObject[QStringLiteral("url")].toString(),
                           jobObject[QStringLiteral("name")].toString(),
                           jobObject[QStringLiteral("description")].toString(),
                           jobObject[QStringLiteral("color")].toString(),
                           jobObject[QStringLiteral("default")].toBool() };

      labels.append(std::move(sLabel));
   }

   emit signalLabelsReceived(labels);
}

void GitHubRestApi::onMilestonesReceived(const QJsonDocument &doc)
{
   QVector<ServerMilestone> milestones;
   const auto labelsArray = doc.array();

   for (auto label : labelsArray)
   {
      const auto jobObject = label.toObject();
      ServerMilestone sMilestone { jobObject[QStringLiteral("id")].toInt(),
                                   jobObject[QStringLiteral("number")].toInt(),
                                   jobObject[QStringLiteral("node_id")].toString(),
                                   jobObject[QStringLiteral("title")].toString(),
                                   jobObject[QStringLiteral("description")].toString(),
                                   jobObject[QStringLiteral("state")].toString() == "open" };

      milestones.append(std::move(sMilestone));
   }

   emit signalMilestonesReceived(milestones);
}

void GitHubRestApi::onIssueCreated(const QJsonDocument &doc)
{
   const auto issue = doc.object();
   const auto url = issue[QStringLiteral("html_url")].toString();

   emit signalIssueCreated(url);
}

void GitHubRestApi::onPullRequestCreated(const QJsonDocument &doc)
{
   const auto issue = doc.object();
   const auto url = issue[QStringLiteral("html_url")].toString();

   emit signalPullRequestCreated(url);
}

void GitHubRestApi::processPullRequets(const QJsonDocument &doc)
{
   const auto prs = doc.array();

   ServerPullRequest prInfo;

   for (const auto &pr : prs)
   {
      prInfo.id = pr["id"].toInt();
      prInfo.title = pr["title"].toString();
      prInfo.body = pr["body"].toString().toUtf8();
      prInfo.head = pr["head"].toObject()["ref"].toString();
      prInfo.base = pr["base"].toObject()["ref"].toString();
      prInfo.isOpen = pr["state"].toString() == "open";
      prInfo.draft = pr["draft"].toBool();
      // prInfo.details;
      prInfo.url = pr["html_url"].toString();

      const auto headObj = pr["head"].toObject();

      prInfo.state.sha = headObj["sha"].toString();

      mPulls.insert(prInfo.state.sha, prInfo);

      auto request = createRequest(QString("commits/%1/status").arg(prInfo.state.sha));
      mManager->get(request);
   }
}

void GitHubRestApi::onPullRequestStatusReceived(const QString &sha, const QJsonDocument &doc)
{
   const auto obj = doc.object();

   mPulls[sha].state.state = obj["state"].toString();

   mPulls[sha].state.eState = mPulls[sha].state.state == "success"
       ? ServerPullRequest::HeadState::State::Success
       : mPulls[sha].state.state == "failure" ? ServerPullRequest::HeadState::State::Failure
                                              : ServerPullRequest::HeadState::State::Pending;
}

void GitHubRestApi::validateData(QNetworkReply *reply)
{
   if (reply == nullptr)
      return;

   const auto data = reply->readAll();
   const auto jsonDoc = QJsonDocument::fromJson(data);
   const auto url = reply->url().path();

   if (jsonDoc.isNull())
   {
      QLog_Error("Ui", QString("Error when parsing Json. Current data:\n%1").arg(QString::fromUtf8(data)));
      return;
   }

   const auto jsonObject = jsonDoc.object();
   if (jsonObject.contains(QStringLiteral("message")))
   {
      const auto message = jsonObject[QStringLiteral("message")].toString();
      QString details;

      if (jsonObject.contains(QStringLiteral("errors")))
      {
         const auto errors = jsonObject[QStringLiteral("errors")].toArray();

         for (auto error : errors)
            details = error[QStringLiteral("message")].toString();
      }

      QLog_Error("Ui", message + ". " + details);

      return;
   }

   if (url.contains("labels"))
      onLabelsReceived(jsonDoc);
   else if (url.contains("milestones"))
      onMilestonesReceived(jsonDoc);
   else if (url.contains("issues/"))
      emit signalIssueUpdated();
   else if (url.contains("issues"))
      onIssueCreated(jsonDoc);
   else if (url.contains("commits"))
   {
      auto sha = url;
      sha.remove("/status");
      onPullRequestStatusReceived(sha.mid(sha.lastIndexOf("/") + 1), jsonDoc);
   }
   else if (url.contains("pulls"))
   {
      if (reply->operation() == QNetworkAccessManager::PutOperation)
         onPullRequestCreated(jsonDoc);
      else if (reply->operation() == QNetworkAccessManager::GetOperation)
         processPullRequets(jsonDoc);
   }
   else
      emit signalConnectionSuccessful();
}
