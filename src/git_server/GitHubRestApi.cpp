#include "GitHubRestApi.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QLogger.h>

using namespace QLogger;

GitHubRestApi::GitHubRestApi(const QString &repoOwner, const QString repoName, const ServerAuthentication &auth,
                             QObject *parent)
   : QObject(parent)
{
   mManager = new QNetworkAccessManager();
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

   const auto reply = mManager->get(request);
   connect(reply, &QNetworkReply::readyRead, this, [this, reply]() {
      if (const auto jsonDoc = validateData(reply); jsonDoc.has_value())
      {
         emit signalConnectionSuccessful();
      }
   });
}

void GitHubRestApi::createIssue() { }

void GitHubRestApi::createPullRequest() { }

void GitHubRestApi::requestLabels()
{
   QNetworkRequest request;
   request.setUrl(formatUrl("labels"));

   const auto reply = mManager->get(request);
   connect(reply, &QNetworkReply::readyRead, this, [this, reply]() { onLabelsReceived(reply); });
}

void GitHubRestApi::getMilestones()
{
   QNetworkRequest request;
   request.setUrl(formatUrl("milestones"));

   const auto reply = mManager->get(request);
   connect(reply, &QNetworkReply::readyRead, this, [this, reply]() { onLabelsReceived(reply); });
}

QUrl GitHubRestApi::formatUrl(const QString endPoint) const
{
   auto tmpUrl = mServerUrl + mRepoOwner + mRepoName + endPoint;
   if (tmpUrl.endsWith("/"))
      tmpUrl = tmpUrl.left(tmpUrl.size() - 1);

   QUrl url(tmpUrl);
   url.setUserName(mAuth.userName);
   url.setPassword(mAuth.userPass);
   return url;
}

void GitHubRestApi::onLabelsReceived(QNetworkReply *reply)
{
   if (const auto jsonDoc = validateData(reply); jsonDoc.has_value())
   {
      QVector<ServerLabel> labels;
      const auto labelsArray = jsonDoc.value().array();

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
}

void GitHubRestApi::onMilestonesReceived(QNetworkReply *reply)
{
   if (const auto jsonDoc = validateData(reply); jsonDoc.has_value())
   {
      QVector<ServerMilestone> milestones;
      const auto labelsArray = jsonDoc.value().array();

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
}

std::optional<QJsonDocument> GitHubRestApi::validateData(QNetworkReply *reply)
{
   if (reply == nullptr)
      return std::nullopt;

   const auto data = reply->readAll();
   const auto jsonDoc = QJsonDocument::fromJson(data);

   if (jsonDoc.isNull())
   {
      QLog_Error("Ui", QString("Error when parsing Json. Current data:\n%1").arg(QString::fromUtf8(data)));
      return std::nullopt;
   }

   const auto jsonObject = jsonDoc.object();
   if (jsonObject.contains(QStringLiteral("message")))
   {
      const auto message = jsonObject[QStringLiteral("message")].toString();

      if (message.contains("Not found", Qt::CaseInsensitive))
         QLog_Error("Ui", QString("Url not found or error when validating user and token."));

      return std::nullopt;
   }

   return jsonDoc;
}
