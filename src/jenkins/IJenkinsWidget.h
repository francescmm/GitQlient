#pragma once

#include <jenkinsplugin_global.h>

#include <QString>
#include <QtPlugin>

#define IJenkinsWidget_iid "francescmm.JenkinsPlugin/0.1.0"

class JENKINSPLUGIN_EXPORT IJenkinsWidget
{
public:
   virtual ~IJenkinsWidget() = default;

   virtual void initialize(const QString &url, const QString &user, const QString &token) = 0;
   virtual void reload() const = 0;
};

Q_DECLARE_INTERFACE(IJenkinsWidget, IJenkinsWidget_iid)
