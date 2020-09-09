#pragma once

#include <JenkinsJobInfo.h>
#include <IFetcher.h>

#include <QFrame>

class QLabel;
class CheckBox;
class QVBoxLayout;
class QHBoxLayout;
class QButtonGroup;
class QRadioButton;
class QTabWidget;
class QNetworkAccessManager;
class QPlainTextEdit;
class QPushButton;
class ButtonLink;

namespace Jenkins
{

class JenkinsJobPanel : public QFrame
{
   Q_OBJECT

public:
   explicit JenkinsJobPanel(const IFetcher::Config &config, QWidget *parent = nullptr);

   void onJobInfoReceived(const JenkinsJobInfo &job);

private:
   IFetcher::Config mConfig;
   QLabel *mName = nullptr;
   ButtonLink *mUrl = nullptr;
   CheckBox *mBuildable = nullptr;
   CheckBox *mInQueue = nullptr;
   QPushButton *mBuild = nullptr;
   QLabel *mHealthDesc = nullptr;
   QFrame *mScrollFrame = nullptr;
   QVBoxLayout *mBuildListLayout = nullptr;
   QHBoxLayout *mLastBuildLayout = nullptr;
   QFrame *mLastBuildFrame = nullptr;
   QTabWidget *mTabWidget = nullptr;
   JenkinsJobInfo mRequestedJob;
   int mTmpBuildsCounter = 0;
   QVector<QWidget *> mTempWidgets;
   QVector<QString> mDownloadedFiles;
   QNetworkAccessManager *mManager = nullptr;
   QMap<int, int> mTabBuildMap;
   QMap<QString, QPair<JobConfigFieldType, QVariant>> mBuildValues;

   void appendJobsData(const QString &jobName, const JenkinsJobBuildInfo &build);
   void fillBuildLayout(const Jenkins::JenkinsJobBuildInfo &build, QHBoxLayout *layout);
   void requestFile(const Jenkins::JenkinsJobBuildInfo &build);
   void storeFile(int buildNumber);
   void findString(QString s, QPlainTextEdit *textEdit);
   void createBuildConfigPanel();
   void triggerBuild();
};
}
