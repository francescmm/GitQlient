#pragma once

#include <IFetcher.h>
#include <JenkinsJobInfo.h>

namespace Jenkins
{

class JobFetcher final : public IFetcher
{
   Q_OBJECT

signals:
   void signalJobsReceived(const QMultiMap<QString, Jenkins::JenkinsJobInfo> &jobs);

public:
   explicit JobFetcher(const IFetcher::Config &config, const QString &jobUrl, bool isCustomUrl,
                       QObject *parent = nullptr);

   void triggerFetch() override;

private:
   QString mJobUrl;
   bool mIsCustomUrl;

   void processData(const QJsonDocument &json) override;
   QMultiMap<QString, JenkinsJobInfo> extractJobs(const QJsonArray &jobs, const QString &viewName);
};
}
