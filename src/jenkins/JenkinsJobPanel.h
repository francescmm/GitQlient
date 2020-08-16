#pragma once

#include <JenkinsJobInfo.h>
#include <IFetcher.h>

#include <QFrame>

class QLabel;
class QCheckBox;
class QVBoxLayout;
class QButtonGroup;
class QRadioButton;

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
   QLabel *mUrl = nullptr;
   QCheckBox *mBuildable = nullptr;
   QCheckBox *mInQueue = nullptr;
   QLabel *mHealthDesc = nullptr;
   QVBoxLayout *mBuildListLayout = nullptr;
   QButtonGroup *mBuildsGroup = nullptr;
   QVector<QRadioButton *> mBuilds;
   JenkinsJobInfo mRequestedJob;
   int mTmpBuildsCounter = 0;

   void appendJobsData(const JenkinsJobBuildInfo &build);
};
}
