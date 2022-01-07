#include "JobFetcher.h"

#include <JobDetailsFetcher.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>

#include <QLogger.h>

using namespace QLogger;

namespace Jenkins
{

JobFetcher::JobFetcher(const Config &config, const QString &jobUrl, bool isCustomUrl, QObject *parent)
   : IFetcher(config, parent)
   , mJobUrl(jobUrl)
   , mIsCustomUrl(isCustomUrl)
{
}

void JobFetcher::triggerFetch()
{
   get(mJobUrl, mIsCustomUrl);
}

void JobFetcher::processData(const QJsonDocument &json)
{
   const auto jsonObject = json.object();
   QMultiMap<QString, JenkinsJobInfo> jobsMap;

   if (jsonObject.contains(QStringLiteral("views")))
   {
      const auto views = jsonObject[QStringLiteral("views")].toArray();

      for (const auto &view : views)
      {
         const auto jobs = view.toObject()[QStringLiteral("jobs")].toArray();

         jobsMap += extractJobs(jobs, view[QStringLiteral("name")].toString());
      }
   }
   else if (jsonObject.contains(QStringLiteral("jobs")))
   {
      const auto jobs = jsonObject[QStringLiteral("jobs")].toArray();

      jobsMap = extractJobs(jobs, "Jobs");
   }

   emit signalJobsReceived(jobsMap);
}

QMultiMap<QString, JenkinsJobInfo> JobFetcher::extractJobs(const QJsonArray &jobs, const QString &viewName)
{
   QMultiMap<QString, JenkinsJobInfo> jobsMap;

   for (const auto &job : jobs)
   {
      QJsonObject jobObject = job.toObject();
      QString url;

      if (jobObject.contains(QStringLiteral("url")))
         url = jobObject[QStringLiteral("url")].toString();

      if (jobObject[QStringLiteral("_class")].toString().contains("WorkflowJob"))
      {
         JenkinsJobInfo jobInfo;
         jobInfo.url = url;

         if (jobObject.contains(QStringLiteral("displayName")))
            jobInfo.name = jobObject[QStringLiteral("displayName")].toString();
         else
         {
            auto labels = url.split("job");
            jobInfo.name = labels.takeLast();
            jobInfo.name.prepend(" >> ");
            jobInfo.name.prepend(labels.takeLast());
            jobInfo.name.remove("/");
         }

         jobInfo.name.replace("%2F", "/");

         if (jobObject.contains(QStringLiteral("color")))
            jobInfo.color = jobObject[QStringLiteral("color")].toString();

         jobsMap.insert(viewName, jobInfo);
      }
   }

   return jobsMap;
}

}
