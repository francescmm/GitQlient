#pragma once

#include <JenkinsJobInfo.h>
#include <IFetcher.h>

#include <QFrame>

class QVBoxLayout;
class QTreeWidgetItem;
class QListWidget;
class QLabel;

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
   void addJobs(const QMultiMap<QString, JenkinsJobInfo> &jobs);
   void showJobInfo(QTreeWidgetItem *item, int column);
   QIcon getIconForJob(JenkinsJobInfo job) const;
   void createHeader(const QString &name, QListWidget *listWidget);
   void onHeaderClicked(QListWidget *listWidget, QLabel *mTagArrow);
};
}
