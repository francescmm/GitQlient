#pragma once

#include <QDateTime>
#include <QString>
#include <QMap>
#include <QMetaType>
#include <QColor>

namespace Jenkins
{

inline QColor resultColor(const QString &result)
{
   if (result == "SUCCESS")
      return QColor("#00AF18");
   else if (result == "UNSTABLE")
      return QColor("#D89000");
   else if (result == "FAILURE" || result == "FAILED")
      return QColor("#FF2222");
   else if (result == "ABORTED")
      return QColor("#5B5B5B");
   else if (result == "NOT_BUILT")
      return QColor("#C8C8C8");

   return QColor("#C8C8C8");
}

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
   bool operator==(const JenkinsJobInfo &info) const { return name == info.name; }

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
   QVector<JenkinsJobBuildInfo> builds;
};

}

Q_DECLARE_METATYPE(Jenkins::JenkinsJobInfo);
