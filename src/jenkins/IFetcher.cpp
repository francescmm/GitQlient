#include "IFetcher.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSettings>
#include <QJsonDocument>

#include <QLogger.h>

using namespace QLogger;

namespace QJenkins
{
IFetcher::IFetcher(const QString &user, const QString &token, QObject *parent)
   : QObject(parent)
   , mUser(user)
   , mToken(token)
   , mAccessManager(new QNetworkAccessManager(this))
{
   connect(mAccessManager, &QNetworkAccessManager::finished, this, &IFetcher::processReply);
}

QNetworkReply *IFetcher::get(const QString &urlStr, int port, bool customUrl)
{
   QSettings settings;

   const auto apiUrl = urlStr.endsWith("api/json") || customUrl ? urlStr : urlStr + "api/json";

   QUrl url(apiUrl);
   url.setPort(port);

   QNetworkRequest request(url);

   const auto userName = settings.value("jenkinsServerUser", "").toString();

   if (!mUser.isEmpty() && !mToken.isEmpty())
      request.setRawHeader(QByteArray("Authorization"),
                           QString("Basic %1:%2").arg(mUser, mToken).toLocal8Bit().toBase64());

   return mAccessManager->get(request);
}

void IFetcher::processReply(QNetworkReply *reply)
{
   const auto data = reply->readAll();

   if (data.isEmpty())
      QLog_Warning("QJenkins", QString("Reply from {%1} is empty.").arg(reply->url().toString()));

   const auto json = QJsonDocument::fromJson(data);

   if (json.isNull())
   {
      QLog_Error("QJenkins", QString("Data from {%1} is not a valid JSON").arg(reply->url().toString()));
      QLog_Trace("QJenkins", QString("Data received:\n%1").arg(QString::fromUtf8(data)));
      return;
   }

   processData(json);
}
}
