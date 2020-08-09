#include <IssuesWidget.h>

#include <IRestApi.h>
#include <CreateIssueDlg.h>
#include <IssueItem.h>
#include <GitQlientSettings.h>
#include <GitConfig.h>
#include <GitHubRestApi.h>
#include <GitLabRestApi.h>

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>

using namespace GitServer;

IssuesWidget::IssuesWidget(const QSharedPointer<GitBase> &git, Config config, QWidget *parent)
   : QFrame(parent)
   , mGit(git)
   , mConfig(config)
{
   GitQlientSettings settings;
   QScopedPointer<GitConfig> gitConfig(new GitConfig(mGit));
   const auto serverUrl = gitConfig->getServerUrl();
   const auto userName = settings.globalValue(QString("%1/user").arg(serverUrl)).toString();
   const auto userToken = settings.globalValue(QString("%1/token").arg(serverUrl)).toString();
   const auto repoInfo = gitConfig->getCurrentRepoAndOwner();
   const auto endpoint = settings.globalValue(QString("%1/endpoint").arg(serverUrl)).toString();

   if (serverUrl.contains("github"))
      mApi = new GitHubRestApi(repoInfo.first, repoInfo.second, { userName, userToken, endpoint });
   else if (serverUrl.contains("gitlab"))
      mApi = new GitLabRestApi(userName, repoInfo.second, serverUrl, { userName, userToken, endpoint });

   const auto headerTitle = new QLabel(mConfig == Config::Issues ? tr("Issues") : tr("Pull Requests"));
   headerTitle->setObjectName("HeaderTitle");

   const auto newIssue = new QPushButton(tr("New issue"));
   newIssue->setObjectName("ButtonIssuesHeaderFrame");
   connect(newIssue, &QPushButton::clicked, this, &IssuesWidget::createNewIssue);

   const auto headerFrame = new QFrame();
   headerFrame->setObjectName("IssuesHeaderFrame");
   const auto headerLayout = new QHBoxLayout(headerFrame);
   headerLayout->setContentsMargins(QMargins());
   headerLayout->setSpacing(0);
   headerLayout->addWidget(headerTitle);
   headerLayout->addStretch();
   headerLayout->addWidget(newIssue);

   mIssuesLayout = new QVBoxLayout();

   const auto footerFrame = new QFrame();
   footerFrame->setObjectName("IssuesFooterFrame");

   const auto issuesLayout = new QVBoxLayout(this);
   issuesLayout->setContentsMargins(QMargins());
   issuesLayout->setSpacing(0);
   issuesLayout->addWidget(headerFrame);
   issuesLayout->addLayout(mIssuesLayout);
   issuesLayout->addWidget(footerFrame);

   const auto timer = new QTimer();
   connect(timer, &QTimer::timeout, this, &IssuesWidget::loadData);
   timer->start(300000);
}

void IssuesWidget::loadData()
{
   connect(mApi, &IRestApi::issuesReceived, this, &IssuesWidget::onIssuesReceived);

   if (mConfig == Config::Issues)
      mApi->requestIssues();
   else
      mApi->requestPullRequests();
}

void IssuesWidget::createNewIssue()
{
   const auto createIssue = new CreateIssueDlg(mGit, this);
   createIssue->exec();
}

void IssuesWidget::onIssuesReceived(const QVector<Issue> &issues)
{
   delete mIssuesWidget;
   delete mScrollArea;

   mIssuesWidget = new QFrame();
   mIssuesWidget->setObjectName("IssuesWidget");
   mIssuesWidget->setStyleSheet("#IssuesWidget{"
                                "background-color: #2E2F30;"
                                "}");
   const auto issuesLayout = new QVBoxLayout(mIssuesWidget);
   issuesLayout->setAlignment(Qt::AlignTop | Qt::AlignVCenter);
   issuesLayout->setContentsMargins(QMargins());
   issuesLayout->setSpacing(0);

   mScrollArea = new QScrollArea();
   mScrollArea->setWidget(mIssuesWidget);
   mScrollArea->setWidgetResizable(true);

   mIssuesLayout->addWidget(mScrollArea);

   auto totalIssues = issues.count();
   auto count = 0;

   for (auto &issue : issues)
   {
      issuesLayout->addWidget(new IssueItem(issue));

      if (count++ < totalIssues - 1)
      {
         const auto separator = new QFrame();
         separator->setObjectName("orangeHSeparator");
         issuesLayout->addWidget(separator);
      }
   }
}
