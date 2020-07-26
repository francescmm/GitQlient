#include <IRestApi.h>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QLogger.h>

using namespace QLogger;

IRestApi::IRestApi(const ServerAuthentication &auth, QObject *parent)
   : QObject(parent)
   , mManager(new QNetworkAccessManager())
   , mAuth(auth)
{
}

std::optional<QJsonDocument> IRestApi::validateData(QNetworkReply *reply, QString &errorString)
{
   const auto data = reply->readAll();
   const auto jsonDoc = QJsonDocument::fromJson(data);
   const auto url = reply->url().path();

   if (jsonDoc.isNull())
   {
      QLog_Error("Ui", QString("Error when parsing Json. Current data:\n%1").arg(QString::fromUtf8(data)));
      return std::nullopt;
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

         errorString = message + ". " + details;

         QLog_Error("Ui", errorString);

         return std::nullopt;
      }
   }
   else if (jsonObject.contains(QStringLiteral("error")))
   {
      errorString = jsonObject[QStringLiteral("error")].toString();

      QLog_Error("Ui", errorString);

      return std::nullopt;
   }

   reply->deleteLater();

   return jsonDoc;
}

QUrl IRestApi::formatUrl(const QString page) const
{
   auto tmpUrl = QString(mAuth.endpointUrl + page);

   if (tmpUrl.endsWith("/"))
      tmpUrl = tmpUrl.left(tmpUrl.size() - 1);

   return QUrl(tmpUrl);
}
