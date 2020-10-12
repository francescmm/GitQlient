#include <IssueDetailedView.h>

#include <IssueItem.h>
#include <CircularPixmap.h>
#include <GitServerCache.h>
#include <IRestApi.h>
#include <PrCommitsList.h>
#include <PrChangesList.h>
#include <PrCommentsList.h>

#include <QMessageBox>
#include <QLocale>
#include <QPushButton>
#include <QToolButton>
#include <QButtonGroup>
#include <QStackedLayout>
#include <QMenu>
#include <QEvent>

using namespace GitServer;

IssueDetailedView::IssueDetailedView(const QSharedPointer<GitBase> &git,
                                     const QSharedPointer<GitServerCache> &gitServerCache, QWidget *parent)
   : QFrame(parent)
   , mGit(git)
   , mGitServerCache(gitServerCache)
   , mBtnGroup(new QButtonGroup())
   , mTitleLabel(new QLabel())
   , mCreationLabel(new QLabel())
   , mStackedLayout(new QStackedLayout())
   , mPrCommentsList(new PrCommentsList(mGitServerCache))
   , mPrChangesList(new PrChangesList(mGit))
   , mPrCommitsList(new PrCommitsList(mGitServerCache))
   , mReviewBtn(new QToolButton())
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
   connect(mBtnGroup, &QButtonGroup::idClicked, mStackedLayout, &QStackedLayout::setCurrentIndex);
#else
   connect(mBtnGroup, SIGNAL(buttonClicked(int)), mStackedLayout, SLOT(setCurrentIndex(int)));
#endif

   const auto actionGroup = new QActionGroup(this);
   const auto reviewMenu = new QMenu(mReviewBtn);
   reviewMenu->installEventFilter(this);
   mReviewBtn->setToolButtonStyle(Qt::ToolButtonIconOnly);
   mReviewBtn->setPopupMode(QToolButton::InstantPopup);
   mReviewBtn->setIcon(QIcon(":/icons/review_comment"));
   mReviewBtn->setToolTip(tr("Start review"));
   mReviewBtn->setDisabled(true);
   mReviewBtn->setMenu(reviewMenu);

   auto action = new QAction(tr("Only comments"));
   action->setToolTip(tr("Comment"));
   action->setCheckable(true);
   action->setChecked(true);
   action->setData(static_cast<int>(ReviewState::None));
   actionGroup->addAction(action);
   reviewMenu->addAction(action);

   action = new QAction(tr("Review: Approve"));
   action->setCheckable(true);
   action->setToolTip(tr("The comments will be part of a review"));
   action->setData(static_cast<int>(ReviewState::Approved));
   actionGroup->addAction(action);
   reviewMenu->addAction(action);

   action = new QAction(tr("Review: Request changes"));
   action->setCheckable(true);
   action->setToolTip(tr("The comments will be part of a review"));
   action->setData(static_cast<int>(ReviewState::RequestChanges));
   actionGroup->addAction(action);
   reviewMenu->addAction(action);

   connect(actionGroup, &QActionGroup::triggered, this, [this](QAction *sender) {
      switch (static_cast<ReviewState>(sender->data().toInt()))
      {
         case ReviewState::None:
            mReviewBtn->setIcon(QIcon(":/icons/review_comment"));
            mReviewBtn->setToolTip(tr("Comment"));
            break;
         case ReviewState::Approved:
            mReviewBtn->setIcon(QIcon(":/icons/review_approve"));
            mReviewBtn->setToolTip(tr("Approved"));
            break;
         case ReviewState::RequestChanges:
            mReviewBtn->setIcon(QIcon(":/icons/review_change"));
            mReviewBtn->setToolTip(tr("Request changes"));
            break;
      }
   });

   mAddComment = new QPushButton();
   mAddComment->setCheckable(true);
   mAddComment->setChecked(false);
   mAddComment->setIcon(QIcon(":/icons/add_comment"));
   mAddComment->setToolTip(tr("Add new comment"));
   mAddComment->setDisabled(true);
   connect(mAddComment, &QPushButton::clicked, mPrCommentsList, &PrCommentsList::addGlobalComment);

   mCloseIssue = new QPushButton();
   mCloseIssue->setCheckable(true);
   mCloseIssue->setChecked(false);
   mCloseIssue->setIcon(QIcon(":/icons/close_issue"));
   mCloseIssue->setToolTip(tr("Close"));
   mCloseIssue->setDisabled(true);
   connect(mCloseIssue, &QPushButton::clicked, this, &IssueDetailedView::closeIssue);

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
   headerLayout->addWidget(mReviewBtn);
   headerLayout->addWidget(mAddComment);
   headerLayout->addWidget(mCloseIssue);

   const auto footerFrame = new QFrame();
   footerFrame->setObjectName("IssuesFooterFrame");

   mPrCommitsList->setVisible(false);

   mStackedLayout->insertWidget(static_cast<int>(Buttons::Comments), mPrCommentsList);
   mStackedLayout->insertWidget(static_cast<int>(Buttons::Changes), mPrChangesList);
   mStackedLayout->insertWidget(static_cast<int>(Buttons::Commits), mPrCommitsList);
   mStackedLayout->setCurrentWidget(mPrCommentsList);

   const auto issuesLayout = new QVBoxLayout(this);
   issuesLayout->setContentsMargins(QMargins());
   issuesLayout->setSpacing(0);
   issuesLayout->addWidget(headerFrame);
   issuesLayout->addLayout(mStackedLayout);
   issuesLayout->addWidget(footerFrame);

   connect(mPrCommentsList, &PrCommentsList::frameReviewLink, mPrChangesList, &PrChangesList::addLinks);
   connect(mPrChangesList, &PrChangesList::gotoReview, this, [this](int frameId) {
      mBtnGroup->button(static_cast<int>(Buttons::Comments))->setChecked(true);
      mStackedLayout->setCurrentIndex(static_cast<int>(Buttons::Comments));
      mPrCommentsList->highlightComment(frameId);
   });
}

IssueDetailedView::~IssueDetailedView()
{
   delete mBtnGroup;
}

void IssueDetailedView::loadData(IssueDetailedView::Config config, int issueNum)
{
   mIssueNumber = issueNum;

   mIssue = config == Config::Issues ? mGitServerCache->getIssue(issueNum) : mGitServerCache->getPullRequest(issueNum);

   mCloseIssue->setIcon(
       QIcon(QString::fromUtf8(config == Config::Issues ? ":/icons/close_issue" : ":/icons/close_pr")));

   const auto title = mIssue.title.count() >= 40 ? mIssue.title.left(40).append("...") : mIssue.title;
   mTitleLabel->setText(QString("#%1 - %2").arg(mIssue.number).arg(title));

   const auto days = mIssue.creation.daysTo(QDateTime::currentDateTime());
   const auto whenText = days <= 30
       ? days != 0 ? QString::fromUtf8(" %1 days ago").arg(days) : QString::fromUtf8(" today")
       : QString(" on %1").arg(mIssue.creation.date().toString(QLocale().dateFormat(QLocale::ShortFormat)));

   mCreationLabel->setText(QString("Created by <b>%1</b>%2").arg(mIssue.creator.name, whenText));
   mCreationLabel->setToolTip(mIssue.creation.toString(QLocale().dateTimeFormat(QLocale::ShortFormat)));
   mCreationLabel->setVisible(true);

   mPrCommentsList->loadData(static_cast<PrCommentsList::Config>(config), issueNum);

   if (config == Config::PullRequests)
   {
      mPrCommitsList->loadData(mIssue.number);

      const auto pr = mGitServerCache->getPullRequest(mIssue.number);
      mPrChangesList->loadData(pr.base, pr.head);
   }

   mBtnGroup->button(static_cast<int>(Buttons::Commits))->setEnabled(config == Config::PullRequests);
   mBtnGroup->button(static_cast<int>(Buttons::Changes))->setEnabled(config == Config::PullRequests);
   mBtnGroup->button(static_cast<int>(Buttons::Comments))->setEnabled(true);
   mReviewBtn->setEnabled(config == Config::PullRequests);
   mCloseIssue->setEnabled(true);
   mAddComment->setEnabled(true);
}

bool IssueDetailedView::eventFilter(QObject *obj, QEvent *event)
{
   if (const auto menu = qobject_cast<QMenu *>(obj); menu && event->type() == QEvent::Show)
   {
      auto localPos = menu->parentWidget()->pos();
      localPos.setX(localPos.x() - menu->width() + menu->parentWidget()->width());
      auto pos = mapToGlobal(localPos);
      menu->show();
      pos.setY(pos.y() + menu->parentWidget()->height());
      menu->move(pos);
      return true;
   }

   return false;
}

void IssueDetailedView::closeIssue()
{
   if (const auto ret = QMessageBox::question(this, tr("Close issue"), tr("Are you sure you want to close the issue?"));
       ret == QMessageBox::Yes)
   {
      mIssue.isOpen = false;
      mGitServerCache->getApi()->updateIssue(mIssue.number, mIssue);
   }
}
