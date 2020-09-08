#include "BuildGeneralInfoFetcher.h"

#include <StageFetcher.h>

#include <GitQlientSettings.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace Jenkins
{

BuildGeneralInfoFetcher::BuildGeneralInfoFetcher(const Config &config, const JenkinsJobBuildInfo &build,
                                                 QObject *parent)
   : IFetcher(config, parent)
   , mBuild(build)
{
}

void BuildGeneralInfoFetcher::triggerFetch()
{
   get(mBuild.url + QString::fromUtf8("api/json"), true);
}

void BuildGeneralInfoFetcher::processData(const QJsonDocument &json)
{
   auto jsonObject = json.object();

   if (jsonObject.contains(QStringLiteral("duration")))
      mBuild.duration = jsonObject[QStringLiteral("duration")].toInt();
   if (jsonObject.contains(QStringLiteral("result")))
      mBuild.result = jsonObject[QStringLiteral("result")].toString();
   if (jsonObject.contains(QStringLiteral("timestamp")))
      mBuild.date = QDateTime::fromMSecsSinceEpoch(jsonObject[QStringLiteral("timestamp")].toInt());

   if (jsonObject.contains(QStringLiteral("culprits")))
   {
      QJsonArray culprits = jsonObject[QStringLiteral("culprits")].toArray();

      for (const auto &item : culprits)
      {
         const auto obj = item.toObject();

         if (obj.contains(QStringLiteral("fullName")))
         {
            mBuild.user = obj[QStringLiteral("fullName")].toString();
            break;
         }
      }
   }

   const auto stagesFetcher = new StageFetcher(mConfig, mBuild);
   connect(stagesFetcher, &StageFetcher::signalStagesReceived, this, &BuildGeneralInfoFetcher::appendStages);
   connect(stagesFetcher, &StageFetcher::signalStagesReceived, stagesFetcher, &StageFetcher::deleteLater);

   stagesFetcher->triggerFetch();
}

void BuildGeneralInfoFetcher::appendStages(const QVector<JenkinsStageInfo> &stages)
{
   mBuild.stages = stages;

   emit signalBuildInfoReceived(mBuild);
}

}
