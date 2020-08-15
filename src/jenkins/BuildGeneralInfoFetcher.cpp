#include "BuildGeneralInfoFetcher.h"

#include <StageFetcher.h>

#include <GitQlientSettings.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace QJenkins
{

BuildGeneralInfoFetcher::BuildGeneralInfoFetcher(const QString &user, const QString &token,
                                                 const JenkinsJobBuildInfo &build)
   : IFetcher(user, token)
   , mBuild(build)
{
}

void BuildGeneralInfoFetcher::triggerFetch()
{
   get(mBuild.url);
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

   const auto stagesFetcher = new StageFetcher(mUser, mToken, mBuild);
   connect(stagesFetcher, &StageFetcher::signalStagesReceived, this, &BuildGeneralInfoFetcher::appendStages);

   stagesFetcher->triggerFetch();
}

void BuildGeneralInfoFetcher::appendStages(const QVector<JenkinsStageInfo> &stages)
{
   mBuild.stages = stages;

   emit signalBuildInfoReceived(mBuild);
}

}
