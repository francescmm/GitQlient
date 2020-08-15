#pragma once

#include <QObject>

#include <QNetworkRequest>

class QNetworkAccessManager;
class QNetworkReply;
class QJsonDocument;

namespace QJenkins
{
class IFetcher : public QObject
{
   Q_OBJECT
public:
   explicit IFetcher(const QString &user, const QString &token, QObject *parent = nullptr);

   virtual void triggerFetch() = 0;

protected:
   QString mUser;
   QString mToken;

   virtual QNetworkReply *get(const QString &urlStr, int port = 443, bool customUrl = false) final;

private:
   QNetworkAccessManager *mAccessManager = nullptr;

   virtual void processReply(QNetworkReply *reply) final;
   virtual void processData(const QJsonDocument &json) = 0;
};
}
