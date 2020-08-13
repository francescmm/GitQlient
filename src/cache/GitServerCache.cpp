#include <GitServerCache.h>

#include <GitQlientSettings.h>
#include <GitConfig.h>
#include <GitHubRestApi.h>
#include <GitLabRestApi.h>

#include <Label.h>
#include <Milestone.h>

using namespace GitServer;

GitServerCache::GitServerCache(QObject *parent)
   : QObject(parent)
{
}

GitServerCache::~GitServerCache() { }

bool GitServerCache::init(const QString &serverUrl, const QPair<QString, QString> &repoInfo)
{
   mInit = true;

   GitQlientSettings settings;
   const auto userName = settings.globalValue(QString("%1/user").arg(serverUrl)).toString();
   const auto userToken = settings.globalValue(QString("%1/token").arg(serverUrl)).toString();
   const auto endpoint = settings.globalValue(QString("%1/endpoint").arg(serverUrl)).toString();

   if (serverUrl.contains("github"))
      mApi.reset(new GitHubRestApi(repoInfo.first, repoInfo.second, { userName, userToken, endpoint }));
   else if (serverUrl.contains("gitlab"))
      mApi.reset(new GitLabRestApi(userName, repoInfo.second, serverUrl, { userName, userToken, endpoint }));
   else
      mInit = false;

   connect(mApi.get(), &IRestApi::labelsReceived, this, &GitServerCache::initLabels);
   connect(mApi.get(), &IRestApi::milestonesReceived, this, &GitServerCache::initMilestones);
   connect(mApi.get(), &IRestApi::issuesReceived, this, &GitServerCache::initIssues);
   connect(mApi.get(), &IRestApi::pullRequestsReceived, this, &GitServerCache::initPullRequests);
   connect(mApi.get(), &IRestApi::commentsReceived, this,
           [this](const Issue &issue) { mIssues[issue.number] = issue; });
   connect(mApi.get(), &IRestApi::reviewsReceived, this,
           [this](const PullRequest &pr) { mPullRequests[pr.number] = pr; });
   connect(mApi.get(), &IRestApi::issueCreated, this, []() {});
   connect(mApi.get(), &IRestApi::pullRequestCreated, this, []() {});
   connect(mApi.get(), &IRestApi::pullRequestCreated, this,
           [this](const PullRequest &pr) { mPullRequests[pr.number] = pr; });

   mApi->requestLabels();
   mApi->requestMilestones();
   mApi->requestIssues();
   mApi->requestPullRequests();

   /*
   connect(mApi.get(), &IRestApi::errorOccurred, this, []() {});
   connect(mApi.get(), &IRestApi::milestonesReceived, this, [](){});
   connect(mApi.get(), &IRestApi::milestonesReceived, this, [](){});
   connect(mApi.get(), &IRestApi::milestonesReceived, this, [](){});
   */

   return mInit;
}

PullRequest GitServerCache::getPullRequest(const QString &sha) const
{
   const auto iter = std::find_if(mPullRequests.constBegin(), mPullRequests.constEnd(),
                                  [sha](const GitServer::PullRequest &pr) { return pr.state.sha == sha; });

   if (iter != mPullRequests.constEnd())
      return *iter;

   return PullRequest();
}

GitServer::Platform GitServerCache::getPlatform() const
{
   if (dynamic_cast<GitHubRestApi *>(mApi.get()))
      return Platform::GitHub;

   return Platform::GitLab;
}

void GitServerCache::initLabels(const QVector<Label> &labels)
{
   mLabels = labels;
}

void GitServerCache::initMilestones(const QVector<Milestone> &milestones)
{
   mMilestones = milestones;
}

void GitServerCache::initIssues(const QVector<Issue> &issues)
{
   for (auto &issue : issues)
      mIssues.insert(issue.number, issue);
}

void GitServerCache::initPullRequests(const QVector<PullRequest> &prs)
{
   for (auto &pr : prs)
      mPullRequests.insert(pr.number, pr);
}
