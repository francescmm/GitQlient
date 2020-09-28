#include <GitServerWidget.h>

#include <ServerConfigDlg.h>
#include <GitServerCache.h>
#include <CreateIssueDlg.h>
#include <CreatePullRequestDlg.h>
#include <GitHubRestApi.h>
#include <GitLabRestApi.h>
#include <IssuesList.h>
#include <PrList.h>
#include <IssueDetailedView.h>
#include <Platform.h>
#include <GitBase.h>
#include <GitQlientSettings.h>

#include <QPushButton>
#include <QToolButton>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QStackedLayout>
#include <QLabel>

using namespace GitServer;

GitServerWidget::GitServerWidget(const QSharedPointer<GitCache> &cache, const QSharedPointer<GitBase> &git,
                                 const QSharedPointer<GitServerCache> &gitServerCache, QWidget *parent)
   : QFrame(parent)
   , mCache(cache)
   , mGit(git)
   , mGitServerCache(gitServerCache)
   , mDetailedView(new IssueDetailedView(mGit, mGitServerCache))
{
}

bool GitServerWidget::configure(const GitServer::ConfigData &config)
{
   if (mConfigured)
      return true;

   if (config.user.isEmpty() || config.token.isEmpty())
   {
      const auto configDlg = new ServerConfigDlg(mGitServerCache, config, this);
      mConfigured = configDlg->exec() == QDialog::Accepted;
   }
   else
      mConfigured = true;

   if (mConfigured)
      createWidget();

   return mConfigured;
}

void GitServerWidget::openPullRequest(int prNumber)
{
   mDetailedView->loadData(IssueDetailedView::Config::PullRequests, prNumber);
}

void GitServerWidget::createWidget()
{
   const auto prLabel = QString::fromUtf8(
       mGitServerCache->getPlatform() == GitServer::Platform::GitHub ? "pull request" : "merge request");

   const auto newIssue = new QPushButton();
   newIssue->setIcon(QIcon(":/icons/new_issue"));
   newIssue->setToolTip("Create a new issue");
   connect(newIssue, &QPushButton::clicked, this, &GitServerWidget::createNewIssue);

   const auto newPr = new QPushButton();
   newPr->setIcon(QIcon(":/icons/new_pr"));
   newPr->setToolTip(tr("Create a new %1").arg(prLabel));
   connect(newPr, &QPushButton::clicked, this, &GitServerWidget::createNewPullRequest);

   const auto refresh = new QPushButton();
   refresh->setIcon(QIcon(":/icons/refresh"));
   refresh->setToolTip(tr("Refresh"));

   const auto buttonsLayout = new QHBoxLayout();
   buttonsLayout->setContentsMargins(QMargins());
   buttonsLayout->setSpacing(10);
   buttonsLayout->addWidget(newIssue);
   buttonsLayout->addWidget(newPr);
   buttonsLayout->addWidget(refresh);
   buttonsLayout->addStretch();

   const auto issues = new IssuesList(mGitServerCache);
   connect(issues, &AGitServerItemList::selected, mDetailedView,
           [this](int issueNum) { mDetailedView->loadData(IssueDetailedView::Config::Issues, issueNum); });

   const auto pullRequests = new PrList(mGitServerCache);
   connect(pullRequests, &AGitServerItemList::selected, this, &GitServerWidget::openPullRequest);

   connect(refresh, &QPushButton::clicked, this, [issues, pullRequests]() {
      issues->refreshData();
      pullRequests->refreshData();
   });

   const auto issuesLayout = new QVBoxLayout();
   issuesLayout->setContentsMargins(QMargins());
   issuesLayout->setSpacing(10);
   issuesLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
   issuesLayout->addWidget(issues);
   issuesLayout->addWidget(pullRequests);

   const auto detailsLayout = new QVBoxLayout();
   detailsLayout->setContentsMargins(QMargins());
   detailsLayout->setSpacing(10);
   detailsLayout->setAlignment(Qt::AlignTop);
   detailsLayout->addWidget(mDetailedView);

   const auto centralLayout = new QGridLayout();
   centralLayout->setContentsMargins(10, 10, 10, 10);
   centralLayout->setSpacing(10);
   centralLayout->addLayout(buttonsLayout, 0, 0, 1, 2);
   centralLayout->setColumnStretch(0, 1);
   centralLayout->setColumnStretch(1, 3);
   centralLayout->addLayout(issuesLayout, 2, 0);
   centralLayout->addLayout(detailsLayout, 2, 1);

   issues->loadData();
   pullRequests->loadData();

   delete layout();
   setLayout(centralLayout);
}

void GitServerWidget::createNewIssue()
{
   const auto createIssue = new CreateIssueDlg(mGitServerCache, this);
   createIssue->exec();
}

void GitServerWidget::createNewPullRequest()
{
   const auto prDlg
       = new CreatePullRequestDlg(mCache, mGitServerCache, mGit->getWorkingDir(), mGit->getCurrentBranch(), this);
   prDlg->exec();
}
