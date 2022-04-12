#pragma once

#include <jenkinsplugin_global.h>

#include <QString>
#include <QWidget>
#include <QtPlugin>

#define IJenkinsWidget_iid "francescmm.JenkinsPlugin/0.1.0"

class JENKINSPLUGIN_EXPORT IJenkinsWidget : public QWidget
{
signals:
   void gotoPullRequest(int prNumber);
   void gotoBranch(const QString &branchName);

public:
   IJenkinsWidget(QWidget *parent = nullptr)
      : QWidget(parent)
   {
   }

   virtual ~IJenkinsWidget() = default;

   virtual void init(const QString &url, const QString &user, const QString &token) = 0;
   virtual void update() const = 0;
   virtual IJenkinsWidget *createWidget() = 0;
};

Q_DECLARE_INTERFACE(IJenkinsWidget, IJenkinsWidget_iid)
