#pragma once

#include <QWidget>

#include <JenkinsViewInfo.h>
#include <IFetcher.h>

class QTabWidget;
class GitBase;

namespace Jenkins
{

class RepoFetcher;
class JenkinsJobPanel;

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
   QTabWidget *mTabWidget = nullptr;
   JenkinsJobPanel *mPanel = nullptr;
   RepoFetcher *mRepoFetcher = nullptr;

   void configureGeneralView(const QVector<JenkinsViewInfo> &views);
};

}
