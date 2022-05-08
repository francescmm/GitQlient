#pragma once

#include <jenkinsplugin_global.h>

#include <QString>
#include <QWidget>
#include <QtPlugin>

#define IJenkinsWidget_iid "francescmm.JenkinsPlugin/0.1.0"

namespace JenkinsPlugin
{
struct ConfigData
{
   QString user;
   QString token;
   QString endPoint;
};
}

class JENKINSPLUGIN_EXPORT IJenkinsWidget : public QWidget
{
   Q_OBJECT

signals:
   void gotoPullRequest(int prNumber);
   void gotoBranch(const QString &branchName);

public:
   virtual ~IJenkinsWidget() = default;

   virtual bool configure(JenkinsPlugin::ConfigData config, const QString &styles) = 0;
   virtual bool isConfigured() const { return mConfigured; }
   virtual void start() = 0;
   virtual void update() const = 0;
   virtual IJenkinsWidget *createWidget() = 0;

protected:
   bool mConfigured = false;
};

Q_DECLARE_INTERFACE(IJenkinsWidget, IJenkinsWidget_iid)
