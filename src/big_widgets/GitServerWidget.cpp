#include <GitServerWidget.h>

#include <ServerConfigDlg.h>
#include <GitConfig.h>
#include <GitQlientSettings.h>
#include <GitBase.h>
#include <CreateIssueDlg.h>
#include <CreatePullRequestDlg.h>

#include <QPushButton>
#include <QHBoxLayout>

GitServerWidget::GitServerWidget(const QSharedPointer<RevisionsCache> &cache, const QSharedPointer<GitBase> &git,
                                 QWidget *parent)
   : QFrame(parent)
   , mCache(cache)
   , mGit(git)
{
}

bool GitServerWidget::configure()
{
   QScopedPointer<GitConfig> gitConfig(new GitConfig(mGit));
   const auto serverUrl = gitConfig->getServerUrl();

   GitQlientSettings settings;
   const auto user = settings.globalValue(QString("%1/user").arg(serverUrl)).toString();
   const auto token = settings.globalValue(QString("%1/token").arg(serverUrl)).toString();

   auto moveOn = true;

   if (user.isEmpty() || token.isEmpty())
   {
      const auto configDlg = new ServerConfigDlg(mGit, { user, token }, this);
      moveOn = configDlg->exec() == QDialog::Accepted;
   }

   if (moveOn)
      createWidget();

   return moveOn;
}

void GitServerWidget::createWidget()
{
   GitQlientSettings settings;
   const auto isUnified = settings.localValue(mGit->getGitQlientSettingsDir(), "GitUnifiedServerView", true).toBool();

   QScopedPointer<GitConfig> gitConfig(new GitConfig(mGit));
   const auto prLabel = gitConfig->getServerUrl().contains("github") ? "pull request" : "merge request";

   mUnifiedView = new QPushButton();
   mUnifiedView->setIcon(QIcon(":/icons/unified_view"));
   mUnifiedView->setIconSize({ 30, 30 });
   mUnifiedView->setObjectName("IconButton");
   mUnifiedView->setCheckable(true);
   mUnifiedView->setChecked(isUnified);
   connect(mUnifiedView, &QPushButton::clicked, this, &GitServerWidget::showUnifiedView);

   mSplitView = new QPushButton();
   mSplitView->setIcon(QIcon(":/icons/split_view"));
   mSplitView->setIconSize({ 30, 30 });
   mSplitView->setCheckable(true);
   mSplitView->setChecked(!isUnified);
   mSplitView->setObjectName("IconButton");
   connect(mSplitView, &QPushButton::clicked, this, &GitServerWidget::showSplitView);

   mNewIssue = new QPushButton(tr("New issue"));
   mNewIssue->setObjectName("NormalButton");
   connect(mNewIssue, &QPushButton::clicked, this, &GitServerWidget::createNewIssue);

   mNewPr = new QPushButton(tr("New %1").arg(QString::fromUtf8(prLabel)));
   mNewPr->setObjectName("NormalButton");
   connect(mNewPr, &QPushButton::clicked, this, &GitServerWidget::createNewPullRequest);

   const auto buttonsLayout = new QHBoxLayout();
   buttonsLayout->setContentsMargins(QMargins());
   buttonsLayout->setSpacing(10);
   buttonsLayout->addWidget(mUnifiedView);
   buttonsLayout->addWidget(mSplitView);
   buttonsLayout->addStretch();
   buttonsLayout->addWidget(mNewIssue);
   buttonsLayout->addWidget(mNewPr);

   const auto separator = new QFrame();
   separator->setObjectName("orangeHSeparator");

   const auto centralFrame = new QFrame();
   const auto centralLayout = new QVBoxLayout(centralFrame);
   centralLayout->setContentsMargins(QMargins());
   centralLayout->setSpacing(10);
   centralLayout->addLayout(buttonsLayout);
   centralLayout->addWidget(separator);
   centralLayout->addStretch();

   const auto mainLayout = new QGridLayout();
   mainLayout->setColumnStretch(0, 1);
   mainLayout->setColumnStretch(1, 3);
   mainLayout->setColumnStretch(2, 1);
   mainLayout->setContentsMargins(QMargins());
   mainLayout->setSpacing(0);
   mainLayout->addWidget(centralFrame, 0, 1);

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
