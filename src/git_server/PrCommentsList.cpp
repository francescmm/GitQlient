#include "PrCommentsList.h"

#include <GitServerCache.h>
#include <GitHubRestApi.h>
#include <GitLabRestApi.h>
#include <CircularPixmap.h>
#include <SourceCodeReview.h>

#include <QNetworkAccessManager>
#include <QVBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QNetworkReply>
#include <QTextEdit>

using namespace GitServer;

PrCommentsList::PrCommentsList(const QSharedPointer<GitServerCache> &gitServerCache, QWidget *parent)
   : QFrame(parent)
   , mGitServerCache(gitServerCache)
   , mManager(new QNetworkAccessManager())
{
   setObjectName("IssuesViewFrame");
}

void PrCommentsList::loadData(PrCommentsList::Config config, const GitServer::Issue &issue)
{
   connect(mGitServerCache.get(), &GitServerCache::issueUpdated, this, &PrCommentsList::processComments,
           Qt::UniqueConnection);
   connect(mGitServerCache.get(), &GitServerCache::prUpdated, this, &PrCommentsList::onReviewsReceived,
           Qt::UniqueConnection);

   mConfig = config;
   mIssueNumber = issue.number;

   delete mIssuesFrame;
   delete mScroll;
   delete layout();

   mIssuesLayout = new QVBoxLayout();
   mIssuesLayout->setContentsMargins(20, 20, 20, 20);
   mIssuesLayout->setAlignment(Qt::AlignTop);
   mIssuesLayout->setSpacing(30);

   mIssuesFrame = new QFrame();
   mIssuesFrame->setObjectName("IssuesViewFrame");
   mIssuesFrame->setLayout(mIssuesLayout);

   mScroll = new QScrollArea();
   mScroll->setWidgetResizable(true);
   mScroll->setWidget(mIssuesFrame);

   const auto aLayout = new QVBoxLayout(this);
   aLayout->setContentsMargins(QMargins());
   aLayout->setSpacing(0);
   aLayout->addWidget(mScroll);

   mIssue = issue;

   const auto creationLayout = new QHBoxLayout();
   creationLayout->setContentsMargins(QMargins());
   creationLayout->setSpacing(5);

   if (!mIssue.assignees.isEmpty())
   {
      creationLayout->addWidget(new QLabel(tr("Assigned to ")));

      auto count = 0;
      const auto totalAssignees = mIssue.assignees.count();
      for (auto &assignee : mIssue.assignees)
      {
         const auto assigneLabel = new QLabel(QString("<b>%1</b>").arg(assignee.name));
         assigneLabel->setObjectName("CreatorLink");
         creationLayout->addWidget(assigneLabel);

         if (count++ < totalAssignees - 1)
            creationLayout->addWidget(new QLabel(", "));
      }
   }

   creationLayout->addStretch();

   for (auto &label : mIssue.labels)
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

   if (!mIssue.milestone.title.isEmpty())
   {
      const auto milestone = new QLabel(QString("%1").arg(mIssue.milestone.title));
      milestone->setObjectName("IssueLabel");
      creationLayout->addWidget(milestone);
   }

   const auto frame = new QFrame();
   frame->setObjectName("IssueIntro");

   const auto layout = new QVBoxLayout(frame);
   layout->setContentsMargins(10, 10, 10, 10);
   layout->setSpacing(10);
   layout->addLayout(creationLayout);

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
   const auto body = new QTextEdit();
   body->setMarkdown(QString::fromUtf8(mIssue.body));
   body->setReadOnly(true);
   body->show();
   const auto height = body->document()->size().height();
   body->setFixedHeight(height);
#else
   const auto body = new QLabel(QString::fromUtf8((mIssue.body)));
   body->setWordWrap(true);
#endif
   layout->addWidget(body);

   mIssuesLayout->addWidget(frame);

   if (mConfig == Config::Issues)
      mGitServerCache->getApi()->requestComments(issue);
   else
      mGitServerCache->getApi()->requestReviews(issue);
}

void PrCommentsList::processComments(const Issue &issue)
{
   disconnect(mGitServerCache.get(), &GitServerCache::issueUpdated, this, &PrCommentsList::processComments);

   if (mIssueNumber != issue.number)
      return;

   for (auto &comment : issue.comments)
   {
      const auto layout = createBubbleForComment(comment);
      mIssuesLayout->addLayout(layout);
   }

   mIssuesLayout->addStretch();
}

QLabel *PrCommentsList::createAvatar(const QString &userName, const QString &avatarUrl) const
{
   const auto fileName
       = QString("%1/%2").arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation), userName);
   const auto avatar = new CircularPixmap(QSize(50, 50));
   avatar->setObjectName("Avatar");

   if (!QFile(fileName).exists())
   {
      QNetworkRequest request;
      request.setUrl(avatarUrl);
      request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
      const auto reply = mManager->get(request);
      connect(reply, &QNetworkReply::finished, this,
              [userName, this, avatar]() { storeCreatorAvatar(avatar, userName); });
   }
   else
   {
      QPixmap img(fileName);

      if (!img.isNull())
      {
         img = img.scaled(50, 50, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

         avatar->setPixmap(img);
      }
   }

   return avatar;
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

void PrCommentsList::storeCreatorAvatar(QLabel *avatar, const QString &fileName) const
{
   const auto reply = qobject_cast<QNetworkReply *>(sender());
   const auto data = reply->readAll();

   QDir dir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
   if (!dir.exists())
      dir.mkpath(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));

   QString path = dir.absolutePath() + "/" + fileName;
   QFile file(path);
   if (file.open(QIODevice::WriteOnly))
   {
      file.write(data);
      file.close();

      QPixmap img(path);

      if (!img.isNull())
      {
         img = img.scaled(50, 50, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

         avatar->setPixmap(img);
      }
   }

   reply->deleteLater();
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

QLayout *PrCommentsList::createBubbleForCodeReviewComments(const GitServer::CodeReview &review, QLayout *commentsLayout)
{
   const auto creationLayout = new QHBoxLayout();
   creationLayout->setContentsMargins(QMargins());
   creationLayout->setSpacing(0);

   const auto header
       = QString("<b>%1</b> (%2) started a review ").arg(review.creator.name, review.association.toLower());

   creationLayout->addWidget(createHeadline(review.creation, header));
   creationLayout->addStretch();

   const auto frame = new QFrame();
   frame->setObjectName("IssueIntro");

   const auto innerLayout = new QVBoxLayout(frame);
   innerLayout->setContentsMargins(10, 10, 10, 10);
   innerLayout->setSpacing(20);
   innerLayout->addLayout(creationLayout);

   const auto code = new SourceCodeReview(review.diff.file, review.diff.diff, review.diff.line);

   innerLayout->addWidget(code);
   innerLayout->addLayout(commentsLayout);

   const auto layout = new QHBoxLayout();
   layout->setContentsMargins(QMargins());
   layout->setSpacing(30);
   layout->addSpacing(30);
   layout->addWidget(createAvatar(review.creator.name, review.creator.avatar));
   layout->addWidget(frame);

   return layout;
}

QLayout *PrCommentsList::createBubbleForCodeReviewInitial(const QVector<GitServer::CodeReview> &reviews)
{
   const auto layout = new QGridLayout();
   layout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
   layout->setContentsMargins(QMargins());
   layout->setSpacing(20);

   auto row = 0;

   for (auto &review : reviews)
   {
      const auto creator = createHeadline(review.creation, QString("<b>%1</b><br/>").arg(review.creator.name));
      creator->setObjectName("CodeReviewAuthor");
      creator->setAlignment(Qt::AlignCenter);
      creator->setToolTip(review.association);

      const auto avatarLayout = new QVBoxLayout();
      avatarLayout->setContentsMargins(QMargins());
      avatarLayout->setSpacing(0);
      avatarLayout->addStretch();
      avatarLayout->addWidget(createAvatar(review.creator.name, review.creator.avatar));
      avatarLayout->addWidget(creator);
      avatarLayout->addStretch();

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
      const auto body = new QTextEdit();
      body->setMarkdown(review.body);
      body->setReadOnly(true);
      body->show();
      const auto height = body->document()->size().height();
      body->setMaximumHeight(height);
#else
      const auto body = new QLabel(review.body);
      body->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
      body->setWordWrap(true);
#endif

      const auto frame = new QFrame();
      frame->setObjectName("CodeReviewComment");

      const auto innerLayout = new QVBoxLayout(frame);
      innerLayout->setContentsMargins(10, 10, 10, 10);
      innerLayout->addWidget(body);

      layout->addLayout(avatarLayout, row, 0);
      layout->addWidget(frame, row, 1);

      ++row;
   }

   return layout;
}

void PrCommentsList::onReviewsReceived(const PullRequest &pr)
{
   if (mIssueNumber != pr.number)
      return;

   disconnect(mGitServerCache.get(), &GitServerCache::prUpdated, this, &PrCommentsList::onReviewsReceived);

   QMultiMap<QDateTime, QLayout *> bubblesMap;

   for (const auto &comment : pr.comments)
   {
      const auto layout = createBubbleForComment(comment);
      bubblesMap.insert(comment.creation, layout);
   }

   for (const auto &review : pr.reviews)
   {
      const auto layouts = new QVBoxLayout();

      if (review.state != QString::fromUtf8("COMMENTED"))
         layouts->addLayout(createBubbleForReview(review));

      createBubbleForCodeReview(review.id, pr.reviewComment, layouts);

      bubblesMap.insert(review.creation, layouts);
   }

   for (auto layout : bubblesMap)
   {
      if (layout)
         mIssuesLayout->addLayout(layout);
   }

   mIssuesLayout->addStretch();
   mIssuesLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Fixed, QSizePolicy::Expanding));
}

void PrCommentsList::createBubbleForCodeReview(int reviewId, QVector<CodeReview> comments, QVBoxLayout *layouts)
{
   QMap<int, QVector<CodeReview>> reviews;
   QVector<int> codeReviewIds;

   auto iter = comments.begin();

   while (iter != comments.end())
   {
      if (iter->reviewId == reviewId)
      {
         codeReviewIds.append(iter->id);
         reviews[iter->id].append(*iter);
      }
      else if (codeReviewIds.contains(iter->replyToId))
         reviews[iter->replyToId].append(*iter);
      comments.erase(iter);
   }

   if (!reviews.isEmpty())
   {
      for (auto &codeReviews : reviews)
      {
         std::sort(codeReviews.begin(), codeReviews.end(),
                   [](const CodeReview &r1, const CodeReview &r2) { return r1.creation < r2.creation; });

         const auto commentsLayout = createBubbleForCodeReviewInitial(codeReviews);
         layouts->addLayout(createBubbleForCodeReviewComments(codeReviews.first(), commentsLayout));
      }
   }
}
