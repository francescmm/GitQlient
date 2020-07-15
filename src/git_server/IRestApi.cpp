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
   if (!mAuth.endpointUrl.endsWith("/"))
      mAuth.endpointUrl.append("/");
}

std::optional<QJsonDocument> IRestApi::validateData(QNetworkReply *reply)
{
   if (reply == nullptr)
      return std::nullopt;

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
      }

      QLog_Error("Ui", message + ". " + details);

      return std::nullopt;
   }

   return jsonDoc;
}
