#pragma once

#include <QWidget>

#include <JenkinsViewInfo.h>
#include <IFetcher.h>

class QTabWidget;
class GitBase;
class QStackedLayout;
class QButtonGroup;
class QHBoxLayout;
class QVBoxLayout;

namespace Jenkins
{

class RepoFetcher;

class JenkinsWidget : public QWidget
{
   Q_OBJECT

public:
   JenkinsWidget(const QSharedPointer<GitBase> &git, QWidget *parent = nullptr);
   ~JenkinsWidget() override = default;

   void reload() const;

private:
   QSharedPointer<GitBase> mGit;
   IFetcher::Config mConfig;
   QStackedLayout *mStackedLayout = nullptr;
   RepoFetcher *mRepoFetcher = nullptr;
   QHBoxLayout *mBodyLayout = nullptr;
   QButtonGroup *mBtnGroup = nullptr;
   QVBoxLayout *mButtonsLayout = nullptr;
   QVector<JenkinsViewInfo> mViews;

   void configureGeneralView(const QVector<JenkinsViewInfo> &views);
};

}
