#include "GitLabRestApi.h"

GitLabRestApi::GitLabRestApi(const ServerAuthentication &auth, QObject *parent)
   : IRestApi(auth, parent)
{
}

void GitLabRestApi::testConnection() { }

void GitLabRestApi::createIssue(const ServerIssue &) { }

void GitLabRestApi::updateIssue(int, const ServerIssue &) { }

void GitLabRestApi::createPullRequest(const ServerPullRequest &) { }

void GitLabRestApi::requestLabels() { }

void GitLabRestApi::requestMilestones() { }

void GitLabRestApi::requestPullRequestsState() { }
