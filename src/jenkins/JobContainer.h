#pragma once

#include <JenkinsJobInfo.h>
#include <IFetcher.h>

#include <QFrame>

class QVBoxLayout;

namespace Jenkins
{

struct JenkinsViewInfo;

class JobContainer : public QFrame
{
   Q_OBJECT

signals:
   void signalJobInfoReceived(const JenkinsJobInfo &job);
   void signalJobAreViews(const QVector<JenkinsViewInfo> &views);

public:
   explicit JobContainer(const IFetcher::Config &config, const JenkinsViewInfo &viewInfo, QWidget *parent = nullptr);

private:
   QVBoxLayout *mLayout = nullptr;

   void addJobs(const QVector<JenkinsJobInfo> &jobs);
   void showJobInfo();
};
}
