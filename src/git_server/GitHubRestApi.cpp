#include "GitHubRestApi.h"
#include <ServerIssue.h>
#include <ServerPullRequest.h>

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
{
   mManager = new QNetworkAccessManager();
   connect(mManager, &QNetworkAccessManager::finished, this, &GitHubRestApi::validateData);

   mServerUrl = "https://api.github.com/repos/";

   if (!mServerUrl.endsWith("/"))
      mServerUrl.append("/");

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
   const auto url = formatUrl("");
   QNetworkRequest request;
   request.setUrl(url);

   mManager->get(request);
}

void GitHubRestApi::createIssue(const ServerIssue &issue)
{
   QJsonDocument doc(issue.toJson());
   const auto data = doc.toJson(QJsonDocument::Compact);

   QNetworkRequest request;
   request.setUrl(formatUrl("issues"));
   request.setRawHeader("User-Agent", "GitQlient v1.2.0");
   request.setRawHeader("X-Custom-User-Agent", "GitQlient v1.2.0");
   request.setRawHeader("Content-Type", "application/json");
   request.setRawHeader("Content-Length", QByteArray::number(data.size()));
   request.setRawHeader(
       QByteArray("Authorization"),
       QByteArray("Basic ")
           + QByteArray(QString(QStringLiteral("%1:%2")).arg(mAuth.userName).arg(mAuth.userPass).toLocal8Bit())
                 .toBase64());

   mManager->post(request, data);
}

void GitHubRestApi::updateIssue(int issueNumber, const ServerIssue &issue)
{
   QJsonDocument doc(issue.toJson());
   const auto data = doc.toJson(QJsonDocument::Compact);

   QNetworkRequest request;
   request.setUrl(formatUrl(QString("issues/%1").arg(issueNumber)));
   request.setRawHeader("User-Agent", "GitQlient v1.2.0");
   request.setRawHeader("X-Custom-User-Agent", "GitQlient v1.2.0");
   request.setRawHeader("Content-Type", "application/json");
   request.setRawHeader("Content-Length", QByteArray::number(data.size()));
   request.setRawHeader(
       QByteArray("Authorization"),
       QByteArray("Basic ")
           + QByteArray(QString(QStringLiteral("%1:%2")).arg(mAuth.userName).arg(mAuth.userPass).toLocal8Bit())
                 .toBase64());

   mManager->post(request, data);
}

void GitHubRestApi::createPullRequest(const ServerPullRequest &pullRequest)
{
   QJsonDocument doc(pullRequest.toJson());
   const auto data = doc.toJson(QJsonDocument::Compact);

   QNetworkRequest request;
   request.setUrl(formatUrl("pulls"));
   request.setRawHeader("User-Agent", "GitQlient v1.2.0");
   request.setRawHeader("X-Custom-User-Agent", "GitQlient v1.2.0");
   request.setRawHeader("Content-Type", "application/json");
   request.setRawHeader("Content-Length", QByteArray::number(data.size()));
   request.setRawHeader(
       QByteArray("Authorization"),
       QByteArray("Basic ")
           + QByteArray(QString(QStringLiteral("%1:%2")).arg(mAuth.userName).arg(mAuth.userPass).toLocal8Bit())
                 .toBase64());

   mManager->post(request, data);
}

void GitHubRestApi::requestLabels()
{
   QNetworkRequest request;
   request.setUrl(formatUrl("labels"));

   mManager->get(request);
}

void GitHubRestApi::getMilestones()
{
   QNetworkRequest request;
   request.setUrl(formatUrl("milestones"));

   mManager->get(request);
}

QUrl GitHubRestApi::formatUrl(const QString endPoint) const
{
   auto tmpUrl = mServerUrl + mRepoOwner + mRepoName + endPoint;
   if (tmpUrl.endsWith("/"))
      tmpUrl = tmpUrl.left(tmpUrl.size() - 1);

   return QUrl(tmpUrl);
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

      if (message.contains("Not found", Qt::CaseInsensitive))
         QLog_Error("Ui", QString("Url not found or error when validating user and token."));

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
   else if (url.contains("pulls"))
      onPullRequestCreated(jsonDoc);
   else
      emit signalConnectionSuccessful();
}
