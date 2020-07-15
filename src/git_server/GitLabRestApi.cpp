#include "GitLabRestApi.h"
#include <GitQlientSettings.h>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

GitLabRestApi::GitLabRestApi(const QString &userName, const ServerAuthentication &auth, QObject *parent)
   : IRestApi(auth, parent)
   , mUserName(userName)
{
}

void GitLabRestApi::configureData(const QString &serverUrl) const
{
   auto request = createRequest("/users?username=%1");
   auto url = request.url();

   QUrlQuery query;
   query.addQueryItem("username", mUserName);
   url.setQuery(query);
   request.setUrl(url);

   const auto reply = mManager->get(request);

   connect(reply, &QNetworkReply::finished, this, [this, serverUrl]() {
      const auto reply = qobject_cast<QNetworkReply *>(sender());
      const auto tmpDoc = validateData(reply);

      if (tmpDoc.has_value())
      {
         const auto doc = tmpDoc.value();
         const auto user = doc.object();
         const auto list = tmpDoc->toVariant().toList().first().toMap();

         GitQlientSettings settings;
         settings.setValue(QString("%1/userId").arg(serverUrl), list.value("id").toInt());
      }
   });
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

void GitLabRestApi::requestLabels() { }

void GitLabRestApi::requestMilestones() { }

void GitLabRestApi::requestPullRequestsState() { }

QNetworkRequest GitLabRestApi::createRequest(const QString &page) const
{
   QNetworkRequest request;
   request.setUrl(formatUrl(page));
   request.setRawHeader("User-Agent", "GitQlient v1.2.0");
   request.setRawHeader("X-Custom-User-Agent", "GitQlient v1.2.0");
   request.setRawHeader("Content-Type", "application/json");
   request.setRawHeader(
       QByteArray("Authorization"),
       QByteArray("Basic ")
           + QByteArray(QString(QStringLiteral("%1:%2")).arg(mAuth.userName).arg(mAuth.userPass).toLocal8Bit())
                 .toBase64());

   return request;
}
