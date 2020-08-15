#pragma once

#include <QFrame>
#include <JenkinsJobInfo.h>

class QVBoxLayout;

namespace QJenkins
{

struct JenkinsViewInfo;

class JobContainer : public QFrame
{
   Q_OBJECT

signals:
   void signalJobInfoReceived(const JenkinsJobInfo &job);

public:
   explicit JobContainer(const QString &user, const QString &token, const JenkinsViewInfo &viewInfo,
                         QWidget *parent = nullptr);

private:
   QVBoxLayout *mLayout = nullptr;

   void addJobs(const QVector<JenkinsJobInfo> &jobs);
   void showJobInfo();
};
}
