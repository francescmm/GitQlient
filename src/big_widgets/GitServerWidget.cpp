#include <GitServerWidget.h>

#include <ServerConfigDlg.h>
#include <GitConfig.h>
#include <GitQlientSettings.h>
#include <GitBase.h>
#include <GitServerCache.h>
#include <CreateIssueDlg.h>
#include <CreatePullRequestDlg.h>
#include <GitHubRestApi.h>
#include <GitLabRestApi.h>
#include <IssuesWidget.h>
#include <IssueDetailedView.h>
#include <Platform.h>

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

void GitServerWidget::createWidget()
{
   const auto prLabel
       = mGitServerCache->getPlatform() == GitServer::Platform::GitHub ? "pull request" : "merge request";

   const auto newIssue = new QPushButton(tr("New issue"));
   newIssue->setObjectName("NormalButton");
   connect(newIssue, &QPushButton::clicked, this, &GitServerWidget::createNewIssue);

   const auto newPr = new QPushButton(tr("New %1").arg(QString::fromUtf8(prLabel)));
   newPr->setObjectName("NormalButton");
   connect(newPr, &QPushButton::clicked, this, &GitServerWidget::createNewPullRequest);

   const auto buttonsLayout = new QHBoxLayout();
   buttonsLayout->setContentsMargins(QMargins());
   buttonsLayout->setSpacing(10);
   buttonsLayout->addStretch();
   buttonsLayout->addWidget(newIssue);
   buttonsLayout->addWidget(newPr);

   const auto separator = new QFrame();
   separator->setObjectName("orangeHSeparator");

   const auto centralFrame = new QFrame();
   const auto centralLayout = new QGridLayout(centralFrame);
   centralLayout->setContentsMargins(QMargins());
   centralLayout->setSpacing(10);
   centralLayout->addLayout(buttonsLayout, 0, 0, 1, 2);
   centralLayout->addWidget(separator, 1, 0, 1, 2);

   const auto detailedView = new IssueDetailedView(mGitServerCache);

   const auto issues = new IssuesWidget(mGitServerCache, IssuesWidget::Config::Issues);
   connect(issues, &IssuesWidget::selected, detailedView,
           [config = IssueDetailedView::Config::Issues, detailedView](const GitServer::Issue &issue) {
              detailedView->loadData(config, issue);
           });

   const auto pullRequests = new IssuesWidget(mGitServerCache, IssuesWidget::Config::PullRequests);
   connect(pullRequests, &IssuesWidget::selected, detailedView,
           [config = IssueDetailedView::Config::PullRequests, detailedView](const GitServer::Issue &issue) {
              detailedView->loadData(config, issue);
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
   detailsLayout->addWidget(detailedView);

   centralLayout->setColumnStretch(0, 1);
   centralLayout->setColumnStretch(1, 2);
   centralLayout->addLayout(issuesLayout, 2, 0);
   centralLayout->addLayout(detailsLayout, 2, 1);

   issues->loadData();
   pullRequests->loadData();

   const auto mainLayout = new QGridLayout();
   mainLayout->setColumnStretch(0, 1);
   mainLayout->setColumnStretch(1, 8);
   mainLayout->setColumnStretch(2, 1);
   mainLayout->setContentsMargins(QMargins());
   mainLayout->setSpacing(0);
   mainLayout->addWidget(centralFrame, 0, 1);

   delete layout();
   setLayout(mainLayout);
}

void GitServerWidget::createNewIssue()
{
   const auto createIssue = new CreateIssueDlg(mGit, this);
   createIssue->exec();
}

void GitServerWidget::createNewPullRequest()
{
   const auto prDlg = new CreatePullRequestDlg(mCache, mGit, this);

   QScopedPointer<GitConfig> gitConfig(new GitConfig(mGit));
   const auto serverUrl = gitConfig->getServerUrl();

   if (serverUrl.contains("github"))
      connect(prDlg, &CreatePullRequestDlg::signalRefreshPRsCache, mCache.get(), &GitCache::refreshPRsCache);

   prDlg->exec();
}
