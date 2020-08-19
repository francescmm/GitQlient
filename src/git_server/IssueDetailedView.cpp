#include <IssueDetailedView.h>

#include <IssueItem.h>
#include <CircularPixmap.h>
#include <GitServerCache.h>
#include <IRestApi.h>
#include <Issue.h>
#include <PrCommitsList.h>
#include <PrCommentsList.h>

#include <QLocale>
#include <QPushButton>
#include <QToolButton>
#include <QButtonGroup>
#include <QStackedLayout>

using namespace GitServer;

IssueDetailedView::IssueDetailedView(const QSharedPointer<GitServerCache> &gitServerCache, QWidget *parent)
   : QFrame(parent)
   , mBtnGroup(new QButtonGroup())
   , mTitleLabel(new QLabel())
   , mCreationLabel(new QLabel())
   , mPrCommentsList(new PrCommentsList(gitServerCache))
{
   mTitleLabel->setText(tr("Detailed View"));
   mTitleLabel->setObjectName("HeaderTitle");

   const auto comments = new QToolButton();
   comments->setIcon(QIcon(":/icons/comments"));
   comments->setObjectName("ViewBtnOption");
   comments->setToolTip(tr("Comments view"));
   comments->setCheckable(true);
   comments->setChecked(true);
   comments->setDisabled(true);
   mBtnGroup->addButton(comments, static_cast<int>(Buttons::Comments));

   const auto changes = new QToolButton();
   changes->setIcon(QIcon(":/icons/changes"));
   changes->setObjectName("ViewBtnOption");
   changes->setToolTip(tr("Changes view"));
   changes->setCheckable(true);
   changes->setDisabled(true);
   mBtnGroup->addButton(changes, static_cast<int>(Buttons::Changes));

   const auto commits = new QToolButton();
   commits->setIcon(QIcon(":/icons/commit"));
   commits->setObjectName("ViewBtnOption");
   commits->setToolTip(tr("Commits view"));
   commits->setCheckable(true);
   commits->setDisabled(true);
   mBtnGroup->addButton(commits, static_cast<int>(Buttons::Commits));

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
   connect(mBtnGroup, &QButtonGroup::idClicked, this, &IssueDetailedView::showView);
#else
   connect(mBtnGroup, SIGNAL(buttonClicked(int)), this, SLOT(showView(int)));
#endif

   const auto addComment = new QToolButton();
   addComment->setDisabled(true);
   addComment->setIcon(QIcon(":/icons/add_comment"));
   addComment->setToolTip(tr("Add new comment"));

   const auto headLine = new QVBoxLayout();
   headLine->setContentsMargins(QMargins());
   headLine->setSpacing(5);
   headLine->addWidget(mTitleLabel);
   headLine->addWidget(mCreationLabel);

   mCreationLabel->setVisible(false);

   const auto headerFrame = new QFrame();
   headerFrame->setObjectName("IssuesHeaderFrameBig");
   const auto headerLayout = new QHBoxLayout(headerFrame);
   headerLayout->setContentsMargins(QMargins());
   headerLayout->setSpacing(10);
   headerLayout->addLayout(headLine);
   headerLayout->addStretch();
   headerLayout->addWidget(comments);
   headerLayout->addWidget(changes);
   headerLayout->addWidget(commits);
   headerLayout->addSpacing(20);
   headerLayout->addWidget(addComment);

   const auto footerFrame = new QFrame();
   footerFrame->setObjectName("IssuesFooterFrame");

   mPrCommitsList = new PrCommitsList(gitServerCache);
   mPrCommitsList->setVisible(false);

   mStackedLayout = new QStackedLayout();
   mStackedLayout->insertWidget(static_cast<int>(Buttons::Comments), mPrCommentsList);
   mStackedLayout->insertWidget(static_cast<int>(Buttons::Changes), new QFrame());
   mStackedLayout->insertWidget(static_cast<int>(Buttons::Commits), mPrCommitsList);
   mStackedLayout->setCurrentWidget(mPrCommentsList);

   const auto issuesLayout = new QVBoxLayout(this);
   issuesLayout->setContentsMargins(QMargins());
   issuesLayout->setSpacing(0);
   issuesLayout->addWidget(headerFrame);
   issuesLayout->addLayout(mStackedLayout);
   issuesLayout->addWidget(footerFrame);
}

IssueDetailedView::~IssueDetailedView()
{
   delete mBtnGroup;
}

void IssueDetailedView::loadData(IssueDetailedView::Config config, const Issue &issue)
{
   mIssue = issue;

   const auto title = issue.title.count() >= 40 ? issue.title.left(40).append("...") : issue.title;
   mTitleLabel->setText(QString("#%1 - %2").arg(issue.number).arg(title));

   const auto days = issue.creation.daysTo(QDateTime::currentDateTime());
   const auto whenText = days <= 30
       ? days != 0 ? QString::fromUtf8(" %1 days ago").arg(days) : QString::fromUtf8(" today")
       : QString(" on %1").arg(issue.creation.date().toString(QLocale().dateFormat(QLocale::ShortFormat)));

   mCreationLabel->setText(QString("Created by <b>%1</b>%2").arg(mIssue.creator.name, whenText));
   mCreationLabel->setToolTip(issue.creation.toString(QLocale().dateTimeFormat(QLocale::ShortFormat)));
   mCreationLabel->setVisible(true);

   mPrCommentsList->loadData(static_cast<PrCommentsList::Config>(config), issue);

   mBtnGroup->button(static_cast<int>(Buttons::Commits))->setEnabled(config == Config::PullRequests);
   mBtnGroup->button(static_cast<int>(Buttons::Comments))->setEnabled(true);
}

void IssueDetailedView::showView(int view)
{
   switch (static_cast<Buttons>(view))
   {
      case Buttons::Commits:
         mPrCommitsList->loadData(mIssue.number);
         break;
      default:
         break;
   }

   mStackedLayout->setCurrentIndex(view);
}
