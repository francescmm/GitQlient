#pragma once

#include <IFetcher.h>
#include <JenkinsJobInfo.h>

namespace QJenkins
{

class StageFetcher : public IFetcher
{
   Q_OBJECT

signals:
   void signalStagesReceived(const QVector<JenkinsStageInfo> stages);

public:
   StageFetcher(const QString &user, const QString &token, const JenkinsJobBuildInfo &build);

   void triggerFetch() override;

private:
   JenkinsJobBuildInfo mBuild;

   void processData(const QJsonDocument &json) override;
};

}
