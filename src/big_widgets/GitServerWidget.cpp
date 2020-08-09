#include <GitServerWidget.h>

#include <ServerConfigDlg.h>
#include <GitConfig.h>
#include <GitQlientSettings.h>
#include <GitBase.h>
#include <CreateIssueDlg.h>
#include <CreatePullRequestDlg.h>
#include <GitHubRestApi.h>
#include <GitLabRestApi.h>
#include <ServerIssue.h>
#include <IssueButton.h>

#include <QPushButton>
#include <QToolButton>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QLabel>

GitServerWidget::GitServerWidget(const QSharedPointer<RevisionsCache> &cache, const QSharedPointer<GitBase> &git,
                                 QWidget *parent)
   : QFrame(parent)
   , mCache(cache)
   , mGit(git)
{
}

bool GitServerWidget::configure()
{
   if (mConfigured)
      return true;

   QScopedPointer<GitConfig> gitConfig(new GitConfig(mGit));
   const auto serverUrl = gitConfig->getServerUrl();

   GitQlientSettings settings;
   const auto user = settings.globalValue(QString("%1/user").arg(serverUrl)).toString();
   const auto token = settings.globalValue(QString("%1/token").arg(serverUrl)).toString();

   if (user.isEmpty() || token.isEmpty())
   {
      const auto configDlg = new ServerConfigDlg(mGit, { user, token }, this);
      mConfigured = configDlg->exec() == QDialog::Accepted;
   }
   else
      mConfigured = true;

   if (mConfigured)
   {
      const auto userName = settings.globalValue(QString("%1/user").arg(serverUrl)).toString();
      const auto userToken = settings.globalValue(QString("%1/token").arg(serverUrl)).toString();
      const auto repoInfo = gitConfig->getCurrentRepoAndOwner();
      const auto endpoint = settings.globalValue(QString("%1/endpoint").arg(serverUrl)).toString();

      if (serverUrl.contains("github"))
         mApi = new GitHubRestApi(repoInfo.first, repoInfo.second, { userName, userToken, endpoint });
      else if (serverUrl.contains("gitlab"))
         mApi = new GitLabRestApi(userName, repoInfo.second, serverUrl, { userName, userToken, endpoint });

      connect(mApi, &IRestApi::issuesReceived, this, &GitServerWidget::onIssuesReceived);

      createWidget();
   }

   return mConfigured;
}

void GitServerWidget::createWidget()
{
   GitQlientSettings settings;
   const auto isUnified = settings.localValue(mGit->getGitQlientSettingsDir(), "GitUnifiedServerView", true).toBool();

   QScopedPointer<GitConfig> gitConfig(new GitConfig(mGit));
   const auto prLabel = gitConfig->getServerUrl().contains("github") ? "pull request" : "merge request";

   mUnifiedView = new QToolButton();
   mUnifiedView->setIcon(QIcon(":/icons/unified_view"));
   mUnifiedView->setIconSize({ 30, 30 });
   mUnifiedView->setObjectName("IconButton");
   mUnifiedView->setCheckable(true);
   mUnifiedView->setChecked(isUnified);
   connect(mUnifiedView, &QPushButton::clicked, this, &GitServerWidget::showUnifiedView);

   mSplitView = new QToolButton();
   mSplitView->setIcon(QIcon(":/icons/split_view"));
   mSplitView->setIconSize({ 30, 30 });
   mSplitView->setCheckable(true);
   mSplitView->setChecked(!isUnified);
   mSplitView->setObjectName("IconButton");
   connect(mSplitView, &QPushButton::clicked, this, &GitServerWidget::showSplitView);

   mNewPr = new QPushButton(tr("New %1").arg(QString::fromUtf8(prLabel)));
   mNewPr->setObjectName("NormalButton");
   connect(mNewPr, &QPushButton::clicked, this, &GitServerWidget::createNewPullRequest);

   const auto buttonsLayout = new QHBoxLayout();
   buttonsLayout->setContentsMargins(QMargins());
   buttonsLayout->setSpacing(10);
   buttonsLayout->addWidget(mUnifiedView);
   buttonsLayout->addWidget(mSplitView);
   buttonsLayout->addStretch();
   buttonsLayout->addWidget(mNewPr);

   const auto separator = new QFrame();
   separator->setObjectName("orangeHSeparator");

   const auto issuesWidget = createIssuesWidget();

   const auto centralFrame = new QFrame();
   const auto centralLayout = new QVBoxLayout(centralFrame);
   centralLayout->setContentsMargins(QMargins());
   centralLayout->setSpacing(10);
   centralLayout->addLayout(buttonsLayout);
   centralLayout->addWidget(separator);
   centralLayout->addWidget(issuesWidget);

   const auto mainLayout = new QGridLayout();
   mainLayout->setColumnStretch(0, 1);
   mainLayout->setColumnStretch(1, 3);
   mainLayout->setColumnStretch(2, 1);
   mainLayout->setContentsMargins(QMargins());
   mainLayout->setSpacing(0);
   mainLayout->addWidget(centralFrame, 0, 1);

   delete layout();
   setLayout(mainLayout);
}

void GitServerWidget::showUnifiedView()
{
   mSplitView->blockSignals(true);
   mSplitView->setChecked(false);
   mSplitView->blockSignals(false);
   mUnifiedView->blockSignals(true);
   mUnifiedView->setChecked(true);
   mUnifiedView->blockSignals(false);
}

void GitServerWidget::showSplitView()
{
   mSplitView->blockSignals(true);
   mSplitView->setChecked(true);
   mSplitView->blockSignals(false);
   mUnifiedView->blockSignals(true);
   mUnifiedView->setChecked(false);
   mUnifiedView->blockSignals(false);
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
      connect(prDlg, &CreatePullRequestDlg::signalRefreshPRsCache, mCache.get(), &RevisionsCache::refreshPRsCache);

   prDlg->exec();
}

QWidget *GitServerWidget::createIssuesWidget()
{
   const auto headerTitle = new QLabel(tr("Issues"));
   headerTitle->setObjectName("HeaderTitle");

   const auto newIssue = new QPushButton(tr("New issue"));
   newIssue->setObjectName("ButtonIssuesHeaderFrame");
   connect(newIssue, &QPushButton::clicked, this, &GitServerWidget::createNewIssue);

   const auto headerFrame = new QFrame();
   headerFrame->setObjectName("IssuesHeaderFrame");
   const auto headerLayout = new QHBoxLayout(headerFrame);
   headerLayout->setContentsMargins(QMargins());
   headerLayout->setSpacing(0);
   headerLayout->addWidget(headerTitle);
   headerLayout->addStretch();
   headerLayout->addWidget(newIssue);

   const auto issuesWidget = new QFrame();
   issuesWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
   issuesWidget->setObjectName("IssuesWidget");
   issuesWidget->setStyleSheet("#IssuesWidget{"
                               "background-color: #2E2F30;"
                               "}");
   mIssuesLayout = new QVBoxLayout(issuesWidget);
   mIssuesLayout->setAlignment(Qt::AlignTop | Qt::AlignVCenter);
   mIssuesLayout->setContentsMargins(QMargins());
   mIssuesLayout->setSpacing(0);

   const auto separator = new QFrame();
   separator->setObjectName("orangeHSeparator");

   const auto scrollArea = new QScrollArea();
   scrollArea->setWidget(issuesWidget);
   scrollArea->setWidgetResizable(true);

   const auto footerFrame = new QFrame();
   footerFrame->setObjectName("IssuesFooterFrame");

   const auto issuesFrame = new QFrame();
   const auto issuesLayout = new QVBoxLayout(issuesFrame);
   issuesLayout->setContentsMargins(QMargins());
   issuesLayout->setSpacing(0);
   issuesLayout->addWidget(headerFrame);
   issuesLayout->addWidget(separator);
   issuesLayout->addWidget(scrollArea);
   issuesLayout->addWidget(footerFrame);

   mApi->requestIssues();

   return issuesFrame;
}

void GitServerWidget::onIssuesReceived(const QVector<ServerIssue> &issues)
{
   auto totalIssues = issues.count();
   auto count = 0;

   for (auto &issue : issues)
   {
      mIssuesLayout->addWidget(new IssueButton(issue));

      if (count++ < totalIssues - 1)
      {
         const auto separator = new QFrame();
         separator->setObjectName("orangeHSeparator");
         mIssuesLayout->addWidget(separator);
      }
   }

   mIssuesLayout->addStretch();
}
