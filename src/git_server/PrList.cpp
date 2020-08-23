#include <PrList.h>

#include <IssueItem.h>
#include <IRestApi.h>
#include <GitServerCache.h>

#include <QLabel>
#include <QIcon>

using namespace GitServer;

PrList::PrList(const QSharedPointer<GitServerCache> &gitServerCache, QWidget *parent)
   : AGitServerItemList(gitServerCache, parent)
{
   mHeaderIconLabel->setPixmap(QIcon(":/icons/pull_request").pixmap(QSize(15, 15)));
   mHeaderTitle->setText("Pull Requests");

   connect(mGitServerCache.get(), &GitServerCache::prReceived, this,
           [this]() { onPullRequestsReceived(mGitServerCache->getPullRequests()); });

   onPullRequestsReceived(mGitServerCache->getPullRequests());
}

void PrList::refreshData()
{
   mGitServerCache->getApi()->requestPullRequests();
}

void PrList::onPullRequestsReceived(const QVector<PullRequest> &pr)
{
   QVector<IssueItem *> items;

   for (auto &issue : pr)
   {
      const auto issueItem = new IssueItem(issue);
      connect(issueItem, &IssueItem::selected, this, &AGitServerItemList::selected);
      items.append(issueItem);
   }

   createContent(items);
}
