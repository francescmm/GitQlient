#pragma once

#include <QMap>
#include <QWidget>

#include <IFetcher.h>
#include <JenkinsViewInfo.h>

class GitBase;
class QStackedLayout;
class QButtonGroup;
class QHBoxLayout;
class QVBoxLayout;
class QTimer;
class GitQlientSettings;

namespace Jenkins
{

class RepoFetcher;
class JobContainer;

class JenkinsWidget : public QWidget
{
   Q_OBJECT

signals:
   void gotoPullRequest(int prNumber);
   void gotoBranch(const QString &branchName);

public:
   JenkinsWidget(const QSharedPointer<GitQlientSettings> &settings, QWidget *parent = nullptr);
   ~JenkinsWidget() override;

   void reload() const;

private:
   IFetcher::Config mConfig;
   QStackedLayout *mStackedLayout = nullptr;
   RepoFetcher *mRepoFetcher = nullptr;
   QHBoxLayout *mBodyLayout = nullptr;
   QButtonGroup *mBtnGroup = nullptr;
   QVBoxLayout *mButtonsLayout = nullptr;
   QVector<JenkinsViewInfo> mViews;
   QMap<QString, JobContainer *> mJobsMap;
   QTimer *mTimer = nullptr;

   void configureGeneralView(const QVector<JenkinsViewInfo> &views);
};

}
