#include "PrCommentsList.h"

#include <GitServerCache.h>
#include <GitHubRestApi.h>
#include <GitLabRestApi.h>
#include <CircularPixmap.h>
#include <SourceCodeReview.h>
#include <AvatarHelper.h>
#include <CodeReviewComment.h>
#include <ButtonLink.hpp>

#include <QNetworkAccessManager>
#include <QVBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QNetworkReply>
#include <QTextEdit>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QPushButton>
#include <QIcon>
#include <QScrollBar>

using namespace GitServer;

PrCommentsList::PrCommentsList(const QSharedPointer<GitServerCache> &gitServerCache, QWidget *parent)
   : QFrame(parent)
   , mGitServerCache(gitServerCache)
   , mManager(new QNetworkAccessManager())
{
   setObjectName("IssuesViewFrame");
}

void PrCommentsList::loadData(PrCommentsList::Config config, int issueNumber)
{
   connect(mGitServerCache.get(), &GitServerCache::issueUpdated, this, &PrCommentsList::processComments,
           Qt::UniqueConnection);
   connect(mGitServerCache.get(), &GitServerCache::prUpdated, this, &PrCommentsList::onReviewsReceived,
           Qt::UniqueConnection);

   mConfig = config;
   mIssueNumber = issueNumber;

   auto issue = config == Config::Issues ? mGitServerCache->getIssue(mIssueNumber)
                                         : mGitServerCache->getPullRequest(mIssueNumber);

   delete mIssuesFrame;
   delete mScroll;
   delete layout();

   mCommentsFrame = nullptr;

   mIssuesLayout = new QVBoxLayout();
   mIssuesLayout->setContentsMargins(QMargins());
   mIssuesLayout->setSpacing(30);

   mInputTextEdit = new QTextEdit();
   mInputTextEdit->setPlaceholderText(tr("Add your comment..."));
   mInputTextEdit->setObjectName("AddReviewInput");

   const auto cancel = new QPushButton(tr("Cancel"));
   const auto add = new QPushButton(tr("Comment"));
   connect(cancel, &QPushButton::clicked, this, [this]() {
      mInputTextEdit->clear();
      mInputFrame->setVisible(false);
   });

   connect(add, &QPushButton::clicked, this, [issue, this]() {
      connect(mGitServerCache.get(), &GitServerCache::issueUpdated, this, &PrCommentsList::processComments,
              Qt::UniqueConnection);
      mGitServerCache->getApi()->addIssueComment(issue, mInputTextEdit->toMarkdown());
      mInputTextEdit->clear();
      mInputFrame->setVisible(false);
   });

   const auto btnsLayout = new QHBoxLayout();
   btnsLayout->setContentsMargins(QMargins());
   btnsLayout->setSpacing(0);
   btnsLayout->addWidget(cancel);
   btnsLayout->addStretch();
   btnsLayout->addWidget(add);

   const auto inputLayout = new QVBoxLayout();
   inputLayout->setContentsMargins(20, 20, 20, 10);
   inputLayout->setSpacing(10);
   inputLayout->addWidget(mInputTextEdit);
   inputLayout->addLayout(btnsLayout);

   mInputFrame = new QFrame();
   mInputFrame->setFixedHeight(200);
   mInputFrame->setLayout(inputLayout);
   mInputFrame->setVisible(false);

   const auto descriptionFrame = new QFrame();
   descriptionFrame->setObjectName("IssueDescription");

   const auto bodyLayout = new QVBoxLayout();
   bodyLayout->setContentsMargins(20, 20, 20, 20);
   bodyLayout->setAlignment(Qt::AlignTop);
   bodyLayout->setSpacing(30);
   bodyLayout->addWidget(descriptionFrame);
   bodyLayout->addLayout(mIssuesLayout);
   bodyLayout->addStretch();

   mIssuesFrame = new QFrame();
   mIssuesFrame->setObjectName("IssuesViewFrame");
   mIssuesFrame->setLayout(bodyLayout);

   mScroll = new QScrollArea();
   mScroll->setWidgetResizable(true);
   mScroll->setWidget(mIssuesFrame);

   const auto aLayout = new QVBoxLayout(this);
   aLayout->setContentsMargins(QMargins());
   aLayout->setSpacing(0);
   aLayout->addWidget(mScroll);
   aLayout->addWidget(mInputFrame);
   // aLayout->addStretch();

   const auto creationLayout = new QHBoxLayout();
   creationLayout->setContentsMargins(QMargins());
   creationLayout->setSpacing(5);

   if (!issue.assignees.isEmpty())
   {
      creationLayout->addWidget(new QLabel(tr("Assigned to ")));

      auto count = 0;
      const auto totalAssignees = issue.assignees.count();
      for (auto &assignee : issue.assignees)
      {
         const auto assigneLabel = new QLabel(QString("<b>%1</b>").arg(assignee.name));
         assigneLabel->setObjectName("CreatorLink");
         creationLayout->addWidget(assigneLabel);

         if (count++ < totalAssignees - 1)
            creationLayout->addWidget(new QLabel(", "));
      }
   }
   else
      creationLayout->addWidget(new QLabel(tr("Unassigned")));

   creationLayout->addStretch();

   for (auto &label : issue.labels)
   {
      auto labelWidget = new QLabel();
      labelWidget->setStyleSheet(QString("QLabel {"
                                         "background-color: #%1;"
                                         "border-radius: 7px;"
                                         "min-height: 15px;"
                                         "max-height: 15px;"
                                         "min-width: 15px;"
                                         "max-width: 15px;}")
                                     .arg(label.colorHex));
      labelWidget->setToolTip(label.name);
      creationLayout->addWidget(labelWidget);
   }

   if (!issue.milestone.title.isEmpty())
   {
      const auto milestone = new QLabel(QString("%1").arg(issue.milestone.title));
      milestone->setObjectName("IssueLabel");
      creationLayout->addWidget(milestone);
   }

   const auto layout = new QVBoxLayout(descriptionFrame);
   layout->setContentsMargins(QMargins());
   layout->setSpacing(10);
   layout->addLayout(creationLayout);

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
   const auto body = new QTextEdit();
   body->setMarkdown(QString::fromUtf8(issue.body));
   body->setReadOnly(true);
   body->show();
   const auto height = body->document()->size().height();
   body->setFixedHeight(height);
#else
   const auto body = new QLabel(QString::fromUtf8((issue.body)));
   body->setWordWrap(true);
#endif
   layout->addWidget(body);

   descriptionFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

   const auto separator = new QFrame();
   separator->setObjectName("orangeHSeparator");

   mIssuesLayout->addWidget(separator);

   if (mConfig == Config::Issues)
      mGitServerCache->getApi()->requestComments(issue);
   else
   {
      const auto pr = mGitServerCache->getPullRequest(mIssueNumber);
      mGitServerCache->getApi()->requestReviews(static_cast<PullRequest>(pr));
   }
}

void PrCommentsList::highlightComment(int frameId)
{
   const auto daFrame = mComments.value(frameId);

   mScroll->ensureWidgetVisible(daFrame);

   const auto animationGoup = new QSequentialAnimationGroup();
   auto animation = new QPropertyAnimation(daFrame, "color");
   animation->setDuration(500);
   animation->setStartValue(QColor("#404142"));
   animation->setEndValue(QColor("#606162"));
   animationGoup->addAnimation(animation);

   animation = new QPropertyAnimation(daFrame, "color");
   animation->setDuration(500);
   animation->setStartValue(QColor("#606162"));
   animation->setEndValue(QColor("#404142"));
   animationGoup->addAnimation(animation);

   animationGoup->start();
}

void PrCommentsList::addGlobalComment()
{
   mInputFrame->setVisible(true);
   mInputTextEdit->setFocus();
}

void PrCommentsList::processComments(const Issue &issue)
{
   disconnect(mGitServerCache.get(), &GitServerCache::issueUpdated, this, &PrCommentsList::processComments);

   if (mIssueNumber != issue.number)
      return;

   delete mCommentsFrame;

   mCommentsFrame = new QFrame();

   mIssuesLayout->addWidget(mCommentsFrame);

   const auto commentsLayout = new QVBoxLayout(mCommentsFrame);
   commentsLayout->setContentsMargins(QMargins());
   commentsLayout->setSpacing(30);

   for (auto &comment : issue.comments)
   {
      const auto layout = createBubbleForComment(comment);
      commentsLayout->addLayout(layout);
   }

   commentsLayout->addStretch();
}

QLabel *PrCommentsList::createHeadline(const QDateTime &dt, const QString &prefix)
{
   const auto days = dt.daysTo(QDateTime::currentDateTime());
   const auto whenText = days <= 30
       ? days != 0 ? QString::fromUtf8(" %1 days ago").arg(days) : QString::fromUtf8(" today")
       : QString(" on %1").arg(dt.date().toString(QLocale().dateFormat(QLocale::ShortFormat)));

   const auto label = prefix.isEmpty() ? new QLabel(whenText) : new QLabel(prefix + whenText);
   label->setToolTip(dt.toString(QLocale().dateFormat(QLocale::ShortFormat)));

   return label;
}

void PrCommentsList::onReviewsReceived(PullRequest pr)
{
   if (mIssueNumber != pr.number)
      return;

   mFrameLinks.clear();
   mComments.clear();

   const auto originalPr = pr;

   QMultiMap<QDateTime, QLayout *> bubblesMap;

   for (const auto &comment : qAsConst(pr.comments))
   {
      const auto layout = createBubbleForComment(comment);
      bubblesMap.insert(comment.creation, layout);
   }

   for (const auto &review : qAsConst(pr.reviews))
   {
      const auto layouts = new QVBoxLayout();

      if (review.state != QString::fromUtf8("COMMENTED"))
      {
         const auto reviewLayout = createBubbleForReview(review);
         layouts->addLayout(reviewLayout);
      }

      auto codeReviewsLayouts = createBubbleForCodeReview(review.id, pr.reviewComment);

      for (auto layout : codeReviewsLayouts)
         layouts->addLayout(layout);

      bubblesMap.insert(review.creation, layouts);
   }

   for (auto layout : bubblesMap)
   {
      if (layout)
         mIssuesLayout->addLayout(layout);
   }

   emit frameReviewLink(originalPr, mFrameLinks);

   mIssuesLayout->addStretch();
   mIssuesLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Fixed, QSizePolicy::Expanding));
}

QLayout *PrCommentsList::createBubbleForComment(const Comment &comment)
{
   const auto creationLayout = new QHBoxLayout();
   creationLayout->setContentsMargins(QMargins());
   creationLayout->setSpacing(0);
   creationLayout->addWidget(new QLabel(tr("Comment by ")));

   const auto creator = new QLabel(QString("<b>%1</b>").arg(comment.creator.name));
   creator->setObjectName("CreatorLink");

   creationLayout->addWidget(creator);
   creationLayout->addWidget(createHeadline(comment.creation));
   creationLayout->addStretch();
   creationLayout->addWidget(new QLabel(comment.association));

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
   const auto body = new QTextEdit();
   body->setMarkdown(comment.body);
   body->setReadOnly(true);
   body->show();
   const auto height = body->document()->size().height();
   body->setMaximumHeight(height);
#else
   const auto body = new QLabel(comment.body);
   body->setWordWrap(true);
#endif

   const auto frame = new QFrame();
   frame->setObjectName("IssueIntro");

   const auto innerLayout = new QVBoxLayout(frame);
   innerLayout->setContentsMargins(10, 10, 10, 10);
   innerLayout->setSpacing(5);
   innerLayout->addLayout(creationLayout);
   innerLayout->addSpacing(20);
   innerLayout->addWidget(body);
   innerLayout->addStretch();

   const auto layout = new QHBoxLayout();
   layout->setContentsMargins(QMargins());
   layout->setSpacing(30);
   layout->addSpacing(30);
   layout->addWidget(createAvatar(comment.creator.name, comment.creator.avatar));
   layout->addWidget(frame);

   return layout;
}

QLayout *PrCommentsList::createBubbleForReview(const Review &review)
{
   const auto frame = new QFrame();
   QString header;

   if (review.state == QString::fromUtf8("CHANGES_REQUESTED"))
   {
      frame->setObjectName("IssueIntroChangesRequested");

      header = QString("<b>%1</b> (%2) requested changes ").arg(review.creator.name, review.association.toLower());
   }
   else if (review.state == QString::fromUtf8("APPROVED"))
   {
      frame->setObjectName("IssueIntroApproved");

      header = QString("<b>%1</b> (%2) approved the PR ").arg(review.creator.name, review.association.toLower());
   }

   const auto creationLayout = new QHBoxLayout();
   creationLayout->setContentsMargins(QMargins());
   creationLayout->setSpacing(0);
   creationLayout->addWidget(createHeadline(review.creation, header));
   creationLayout->addStretch();

   const auto innerLayout = new QVBoxLayout(frame);
   innerLayout->setContentsMargins(10, 10, 10, 10);
   innerLayout->setSpacing(20);
   innerLayout->addLayout(creationLayout);

   const auto layout = new QHBoxLayout();
   layout->setContentsMargins(QMargins());
   layout->setSpacing(30);
   layout->addSpacing(30);
   layout->addWidget(createAvatar(review.creator.name, review.creator.avatar));
   layout->addWidget(frame);

   return layout;
}

QVector<QLayout *> PrCommentsList::createBubbleForCodeReview(int reviewId, QVector<CodeReview> &comments)
{
   QMap<int, QVector<CodeReview>> reviews;
   QVector<int> codeReviewIds;
   QVector<QLayout *> listOfCodeReviews;

   auto iter = comments.begin();

   while (iter != comments.end())
   {
      if (iter->reviewId == reviewId)
      {
         codeReviewIds.append(iter->id);
         reviews[iter->id].append(*iter);
         comments.erase(iter);
      }
      else if (codeReviewIds.contains(iter->replyToId))
      {
         reviews[iter->replyToId].append(*iter);
         comments.erase(iter);
      }
      else
         ++iter;
   }

   if (!reviews.isEmpty())
   {
      for (auto &codeReviews : reviews)
      {
         std::sort(codeReviews.begin(), codeReviews.end(),
                   [](const CodeReview &r1, const CodeReview &r2) { return r1.creation < r2.creation; });

         const auto review = codeReviews.constFirst();

         const auto creationLayout = new QHBoxLayout();
         creationLayout->setContentsMargins(QMargins());
         creationLayout->setSpacing(0);

         const auto header
             = QString("<b>%1</b> (%2) started a review ").arg(review.creator.name, review.association.toLower());

         creationLayout->addWidget(createHeadline(review.creation, header));
         creationLayout->addStretch();

         const auto codeReviewFrame = new QFrame();
         const auto frame = new HighlightningFrame();

         const auto innerLayout = new QVBoxLayout(frame);
         innerLayout->setContentsMargins(10, 10, 10, 10);
         innerLayout->setSpacing(20);
         innerLayout->addLayout(creationLayout);

         const auto codeReviewLayout = new QVBoxLayout(codeReviewFrame);

         innerLayout->addWidget(codeReviewFrame);

         const auto code = new SourceCodeReview(review.diff.file, review.diff.diff, review.diff.line);

         codeReviewLayout->addWidget(code);
         codeReviewLayout->addSpacing(20);

         const auto commentsLayout = new QVBoxLayout();
         commentsLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
         commentsLayout->setContentsMargins(QMargins());
         commentsLayout->setSpacing(20);

         for (auto &comment : codeReviews)
            commentsLayout->addWidget(new CodeReviewComment(comment));

         codeReviewLayout->addLayout(commentsLayout);

         if (review.outdated)
         {
            const auto outdatedLabel = new ButtonLink(tr("Outdated"));
            outdatedLabel->setObjectName("OutdatedLabel");
            creationLayout->addWidget(outdatedLabel);

            codeReviewFrame->setVisible(false);

            connect(outdatedLabel, &ButtonLink::clicked, this,
                    [codeReviewFrame]() { codeReviewFrame->setVisible(!codeReviewFrame->isVisible()); });
         }
         else
         {
            const auto addComment = new QPushButton();
            addComment->setCheckable(true);
            addComment->setChecked(false);
            addComment->setIcon(QIcon(":/icons/add_comment"));
            addComment->setToolTip(tr("Add new comment"));

            creationLayout->addWidget(addComment);

            const auto inputTextEdit = new QTextEdit();
            inputTextEdit->setPlaceholderText(tr("Add your comment..."));
            inputTextEdit->setObjectName("AddReviewInput");

            const auto cancel = new QPushButton(tr("Cancel"));
            const auto add = new QPushButton(tr("Comment"));
            const auto btnsLayout = new QHBoxLayout();
            btnsLayout->setContentsMargins(QMargins());
            btnsLayout->setSpacing(0);
            btnsLayout->addWidget(cancel);
            btnsLayout->addStretch();
            btnsLayout->addWidget(add);

            const auto inputFrame = new QFrame();
            inputFrame->setVisible(false);
            const auto inputLayout = new QVBoxLayout(inputFrame);
            inputLayout->setContentsMargins(QMargins());
            inputLayout->addSpacing(20);
            inputLayout->setSpacing(10);
            inputLayout->addWidget(inputTextEdit);
            inputLayout->addLayout(btnsLayout);

            codeReviewLayout->addWidget(inputFrame);

            connect(cancel, &QPushButton::clicked, this, [inputTextEdit, addComment]() {
               inputTextEdit->clear();
               addComment->toggle();
            });
            connect(add, &QPushButton::clicked, this, []() {});
            connect(addComment, &QPushButton::toggled, this, [inputFrame, inputTextEdit](bool checked) {
               inputFrame->setVisible(checked);
               inputTextEdit->setFocus();
            });
         }

         mFrameLinks.insert(mCommentId, review.id);
         mComments.insert(mCommentId, frame);

         ++mCommentId;

         const auto layout = new QHBoxLayout();
         layout->setContentsMargins(QMargins());
         layout->setSpacing(30);
         layout->addSpacing(30);
         layout->addWidget(createAvatar(review.creator.name, review.creator.avatar));
         layout->addWidget(frame);

         listOfCodeReviews.append(layout);
      }
   }

   return listOfCodeReviews;
}

HighlightningFrame::HighlightningFrame(QWidget *parent)
   : QFrame(parent)
{
   setObjectName("IssueIntro");
}

void HighlightningFrame::setColor(QColor color)
{
   setStyleSheet(QString("#IssueIntro { background-color: rgb(%1, %2, %3); }")
                     .arg(color.red())
                     .arg(color.green())
                     .arg(color.blue()));
}

QColor HighlightningFrame::color()
{
   return Qt::black; // getter is not really needed for now
}
