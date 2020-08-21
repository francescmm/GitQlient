#include "RepoFetcher.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QLogger.h>

using namespace QLogger;

namespace Jenkins
{

RepoFetcher::RepoFetcher(const IFetcher::Config &config, const QString &url, QObject *parent)
   : IFetcher(config, parent)
   , mUrl(url)
{
}

void RepoFetcher::triggerFetch()
{
   get(mUrl);
}

void RepoFetcher::processData(const QJsonDocument &json)
{
   const auto jsonObject = json.object();

   if (!jsonObject.contains(QStringLiteral("views")))
   {
      QLog_Info("Jenkins", "Views are absent.");
      return;
   }

   const auto views = jsonObject[QStringLiteral("views")].toArray();
   QVector<JenkinsViewInfo> viewsInfo;
   viewsInfo.reserve(views.count());

   for (const auto &job : views)
   {
      const auto jobObject = job.toObject();
      JenkinsViewInfo info;
      if (jobObject.contains(QStringLiteral("name")))
         info.name = jobObject[QStringLiteral("name")].toString();
      if (jobObject.contains(QStringLiteral("url")))
         info.url = jobObject[QStringLiteral("url")].toString();
      viewsInfo.append(info);
   }

   emit signalViewsReceived(viewsInfo);
}

}
