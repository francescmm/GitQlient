#pragma once

#include <IFetcher.h>
#include <JenkinsJobInfo.h>

namespace QJenkins
{

class BuildGeneralInfoFetcher : public IFetcher
{
   Q_OBJECT

signals:
   void signalBuildInfoReceived(const JenkinsJobBuildInfo &buildInfo);

public:
   BuildGeneralInfoFetcher(const QString &user, const QString &token, const JenkinsJobBuildInfo &build);

   void triggerFetch() override;

private:
   JenkinsJobBuildInfo mBuild;

   void processData(const QJsonDocument &json) override;
   void appendStages(const QVector<JenkinsStageInfo> &stages);
};

}
