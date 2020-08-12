#include <IssuesWidget.h>

#include <IRestApi.h>
#include <CreateIssueDlg.h>
#include <IssueItem.h>
#include <GitQlientSettings.h>
#include <GitConfig.h>
#include <GitHubRestApi.h>
#include <GitLabRestApi.h>
#include <ClickableFrame.h>

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>
#include <QSpinBox>

using namespace GitServer;

IssuesWidget::IssuesWidget(const QSharedPointer<GitBase> &git, Config config, QWidget *parent)
   : QFrame(parent)
   , mGit(git)
   , mConfig(config)
   , mArrow(new QLabel())
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

   const auto pagesSpinBox = new QSpinBox(this);
   connect(pagesSpinBox, SIGNAL(valueChanged(int)), this, SLOT(loadPage(int)));

   const auto pagesLabel = new QLabel(tr("Page: "));

   connect(mApi, &IRestApi::issuesReceived, this, &IssuesWidget::onIssuesReceived);
   connect(mApi, &IRestApi::pullRequestsReceived, this, &IssuesWidget::onPullRequestsReceived);
   connect(mApi, &IRestApi::paginationPresent, this, [pagesSpinBox, pagesLabel](int current, int, int total) {
      if (total != 0)
      {
         pagesSpinBox->blockSignals(true);
         pagesSpinBox->setRange(1, total);
         pagesSpinBox->setValue(current);
         pagesSpinBox->setVisible(true);
         pagesLabel->setVisible(true);
         pagesSpinBox->blockSignals(false);
      }
   });

   const auto headerTitle = new QLabel(mConfig == Config::Issues ? tr("Issues") : tr("Pull Requests"));
   headerTitle->setObjectName("HeaderTitle");

   const auto headerFrame = new ClickableFrame();
   headerFrame->setObjectName("IssuesHeaderFrame");
   connect(headerFrame, &ClickableFrame::clicked, this, &IssuesWidget::onHeaderClicked);

   mArrow->setPixmap(QIcon(":/icons/arrow_up").pixmap(QSize(15, 15)));

   const auto headerLayout = new QHBoxLayout(headerFrame);
   headerLayout->setContentsMargins(QMargins());
   headerLayout->setSpacing(0);
   headerLayout->addWidget(headerTitle);
   headerLayout->addStretch();
   headerLayout->addWidget(mArrow);

   mIssuesLayout = new QVBoxLayout();

   const auto footerFrame = new QFrame();
   footerFrame->setObjectName("IssuesFooterFramePagination");

   pagesSpinBox->setVisible(false);

   const auto footerLayout = new QHBoxLayout(footerFrame);
   footerLayout->setContentsMargins(5, 5, 5, 5);
   footerLayout->addWidget(pagesLabel);
   footerLayout->addWidget(pagesSpinBox);
   footerLayout->addStretch();

   const auto issuesLayout = new QVBoxLayout(this);
   issuesLayout->setContentsMargins(QMargins());
   issuesLayout->setSpacing(0);
   issuesLayout->setAlignment(Qt::AlignTop);
   issuesLayout->addWidget(headerFrame);
   issuesLayout->addLayout(mIssuesLayout);
   issuesLayout->addWidget(footerFrame);

   const auto timer = new QTimer();
   connect(timer, &QTimer::timeout, this, &IssuesWidget::loadData);
   timer->start(300000);
}

void IssuesWidget::loadData()
{
   loadPage();
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
      const auto issueItem = new IssueItem(issue);
      connect(issueItem, &IssueItem::selected, this, &IssuesWidget::selected);
      issuesLayout->addWidget(issueItem);

      if (count++ < totalIssues - 1)
      {
         const auto separator = new QFrame();
         separator->setObjectName("orangeHSeparator");
         issuesLayout->addWidget(separator);
      }
   }

   const auto icon = QIcon(mScrollArea->isVisible() ? QString(":/icons/arrow_up") : QString(":/icons/arrow_down"));
   mArrow->setPixmap(icon.pixmap(QSize(15, 15)));

   setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
}

void IssuesWidget::onPullRequestsReceived(const QVector<PullRequest> &pr)
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

   auto totalIssues = pr.count();
   auto count = 0;

   for (auto &issue : pr)
   {
      const auto issueItem = new IssueItem(issue);
      connect(issueItem, &IssueItem::selected, this, &IssuesWidget::selected);
      issuesLayout->addWidget(issueItem);

      if (count++ < totalIssues - 1)
      {
         const auto separator = new QFrame();
         separator->setObjectName("orangeHSeparator");
         issuesLayout->addWidget(separator);
      }
   }

   const auto icon = QIcon(mScrollArea->isVisible() ? QString(":/icons/arrow_up") : QString(":/icons/arrow_down"));
   mArrow->setPixmap(icon.pixmap(QSize(15, 15)));

   setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
}

void IssuesWidget::onHeaderClicked()
{
   if (mScrollArea)
   {
      const auto issuesVisible = mScrollArea->isVisible();

      mScrollArea->setWidgetResizable(issuesVisible);

      const auto icon = QIcon(issuesVisible ? QString(":/icons/arrow_up") : QString(":/icons/arrow_down"));
      mArrow->setPixmap(icon.pixmap(QSize(15, 15)));
      mScrollArea->setVisible(!issuesVisible);
      /*
      GitQlientSettings settings;
      settings.setLocalValue(mGit->getGitQlientSettingsDir(), "TagsHeader", !tagsAreVisible);
      */

      if (issuesVisible)
         setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
      else
         setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
   }
}

void IssuesWidget::loadPage(int page)
{
   if (page == -1)
   {
      if (mConfig == Config::Issues)
         mApi->requestIssues();
      else
         mApi->requestPullRequests();
   }
   else
   {
      if (mConfig == Config::Issues)
         mApi->requestIssues(page);
      else
         mApi->requestPullRequests(page);
   }
}
