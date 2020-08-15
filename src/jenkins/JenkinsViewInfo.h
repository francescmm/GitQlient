#pragma once

#include <QString>

namespace QJenkins
{

struct JenkinsViewInfo
{
   QString url;
   QString name;

   bool operator==(const JenkinsViewInfo &v) const { return url == v.url && name == v.name; }
};
}
