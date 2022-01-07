#pragma once

#include <QString>

namespace Jenkins
{

struct JenkinsViewInfo
{
   QString url;
   QString name;
   bool isCustomUrl = false;

   bool operator==(const JenkinsViewInfo &v) const { return url == v.url && name == v.name; }
};
}
