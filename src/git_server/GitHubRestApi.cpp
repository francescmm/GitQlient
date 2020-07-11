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
   : ServerRestApi(parent)
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

void GitHubRestApi::getMilestones() { }

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
