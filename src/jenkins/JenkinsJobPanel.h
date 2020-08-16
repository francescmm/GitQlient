#pragma once

#include <QFrame>
#include <JenkinsJobInfo.h>

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
   explicit JenkinsJobPanel(const QString &user, const QString &token, QWidget *parent = nullptr);

   void onJobInfoReceived(const JenkinsJobInfo &job);

private:
   QString mUser;
   QString mToken;
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
