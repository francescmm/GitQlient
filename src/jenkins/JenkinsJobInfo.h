#pragma once

#include <QDateTime>
#include <QString>
#include <QMap>

namespace QJenkins
{

struct JenkinsStageInfo
{
   int id;
   QString name;
   QString url;
   int duration;
   QString result;
};

struct JenkinsJobBuildInfo
{
   bool operator==(const JenkinsJobBuildInfo &build) const { return url == build.url; }

   int number;
   QString url;
   QDateTime date;
   int duration;
   QString result;
   QString user;

   QVector<JenkinsStageInfo> stages;
};

struct JenkinsJobInfo
{
   bool operator==(const JenkinsJobInfo &info) const { return url == info.url; }

   struct HealthStatus
   {
      QString score;
      QString description;
      QString iconClassName;
   };

   QString name;
   QString url;
   QString color;
   bool buildable;
   bool inQueue;
   HealthStatus healthStatus;
   QMap<int, JenkinsJobBuildInfo> builds;
};

}
