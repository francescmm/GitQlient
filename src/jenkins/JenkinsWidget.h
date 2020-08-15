#pragma once

#include <QWidget>

#include <JenkinsViewInfo.h>

class QTabWidget;
class GitBase;

namespace QJenkins
{

class RepoFetcher;
class JenkinsJobPanel;

class JenkinsWidget : public QWidget
{
   Q_OBJECT

public:
   JenkinsWidget(const QSharedPointer<GitBase> &git, QWidget *parent = nullptr);
   ~JenkinsWidget() override = default;

private:
   QSharedPointer<GitBase> mGit;
   QTabWidget *mTabWidget = nullptr;
   JenkinsJobPanel *mPanel = nullptr;
   RepoFetcher *mRepoFetcher = nullptr;

   void configureGeneralView(const QVector<JenkinsViewInfo> &views);
};

}
