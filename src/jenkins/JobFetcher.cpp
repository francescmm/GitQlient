#include "JobFetcher.h"

#include <JobDetailsFetcher.h>

#include <QNetworkAccessManager>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

#include <QLogger.h>

using namespace QLogger;

namespace Jenkins
{

JobFetcher::JobFetcher(const QString &user, const QString &token, const QString &jobUrl, QObject *parent)
   : IFetcher(user, token, parent)
   , mJobUrl(jobUrl)
{
}

void JobFetcher::triggerFetch()
{
   mJobsToUpdated = -1;

   get(mJobUrl);
}

void JobFetcher::processData(const QJsonDocument &json)
{
   const auto jsonObject = json.object();

   if (!jsonObject.contains(QStringLiteral("jobs")))
      QLog_Debug("Jenkins", "Jobs not found.");

   const auto array = jsonObject[QStringLiteral("jobs")].toArray();

   for (const auto &job : array)
   {
      QJsonObject jobObject = job.toObject();
      JenkinsJobInfo jobInfo;

      if (jobObject.contains(QStringLiteral("name")))
         jobInfo.name = jobObject[QStringLiteral("name")].toString();
      if (jobObject.contains(QStringLiteral("url")))
         jobInfo.url = jobObject[QStringLiteral("url")].toString();
      if (jobObject.contains(QStringLiteral("color")))
         jobInfo.color = jobObject[QStringLiteral("color")].toString();

      mJobs.append(jobInfo);
   }

   mJobsToUpdated = mJobs.count();

   for (const auto &jobInfo : mJobs)
   {
      const auto jobRequest = new JobDetailsFetcher(mUser, mToken, jobInfo);
      connect(jobRequest, &JobDetailsFetcher::signalJobDetailsRecieved, this, &JobFetcher::updateJobs);
      connect(jobRequest, &JobDetailsFetcher::signalJobDetailsRecieved, jobRequest, &JobDetailsFetcher::deleteLater);

      jobRequest->triggerFetch();
   }
}

void JobFetcher::updateJobs(const JenkinsJobInfo &updatedInfo)
{
   auto iter = std::find(mJobs.begin(), mJobs.end(), updatedInfo);

   iter->builds = updatedInfo.builds;

   --mJobsToUpdated;

   if (mJobsToUpdated == 0)
   {
      emit signalJobsReceived(mJobs);
   }
}

}
