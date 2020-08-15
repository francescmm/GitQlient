#include "JobDetailsFetcher.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace QJenkins
{

JobDetailsFetcher::JobDetailsFetcher(const QString &user, const QString &token, const JenkinsJobInfo &info)
   : IFetcher(user, token)
   , mInfo(info)
{
}

void JobDetailsFetcher::triggerFetch()
{
   get(mInfo.url);
}

void JobDetailsFetcher::processData(const QJsonDocument &json)
{
   auto jsonObject = json.object();

   readHealthReportsPartFor(jsonObject);
   readBuildsListFor(jsonObject);
   readBuildableFlagFor(jsonObject);
   readIsQueuedFlagFor(jsonObject);

   emit signalJobDetailsRecieved(mInfo);
}

void JobDetailsFetcher::readHealthReportsPartFor(QJsonObject &jsonObject)
{
   if (jsonObject.contains(QStringLiteral("healthReport")))
   {
      QJsonArray healthArray = jsonObject[QStringLiteral("healthReport")].toArray();
      for (const auto &item : healthArray)
      {
         JenkinsJobInfo::HealthStatus status;
         QJsonObject healthObject = item.toObject();
         if (healthObject.contains(QStringLiteral("score")))
            status.score = healthObject[QStringLiteral("score")].toInt();
         if (healthObject.contains(QStringLiteral("description")))
            status.description = healthObject[QStringLiteral("description")].toString();
         if (healthObject.contains(QStringLiteral("iconClassName")))
            status.iconClassName = healthObject[QStringLiteral("iconClassName")].toString();

         mInfo.healthStatus = status;
      }
   }
}

void JobDetailsFetcher::readBuildsListFor(QJsonObject &jsonObject)
{
   if (jsonObject.contains(QStringLiteral("builds")))
   {
      const auto buildsArray = jsonObject[QStringLiteral("builds")].toArray();

      for (const auto &build : buildsArray)
      {
         const auto buildObject = build.toObject();
         if (buildObject.contains(QStringLiteral("number")) && buildObject.contains(QStringLiteral("url")))
         {
            JenkinsJobBuildInfo build;
            build.number = buildObject[QStringLiteral("number")].toInt();
            build.url = buildObject[QStringLiteral("url")].toString();

            mInfo.builds.insert(build.number, build);
         }
      }
   }
}

void JobDetailsFetcher::readBuildableFlagFor(QJsonObject &)
{
   /*
   if (jsonObject.contains(QStringLiteral("buildable")))
      job.setIsBuildable(jsonObject[QStringLiteral("buildable")].toBool());
   else
      job.setIsBuildable(true);
*/
}

void JobDetailsFetcher::readIsQueuedFlagFor(QJsonObject &)
{
   /*
    if (jsonObject.contains(QStringLiteral("inQueue")))
       job.setIsQueued(jsonObject[QStringLiteral("inQueue")].toBool());
    else
       job.setIsQueued(false);
*/
}

}
