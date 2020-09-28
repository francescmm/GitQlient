#include <CodeReviewComment.h>

#include <Comment.h>
#include <AvatarHelper.h>

#include <QVBoxLayout>
#include <QTextEdit>
#include <QLabel>
#include <QLocale>

CodeReviewComment::CodeReviewComment(const GitServer::CodeReview &review, QWidget *parent)
   : QFrame(parent)
{
   const auto creator = createHeadline(review.creation, QString("<b>%1</b><br/>").arg(review.creator.name));
   creator->setObjectName("CodeReviewAuthor");
   creator->setAlignment(Qt::AlignCenter);

   const auto avatarLayout = new QVBoxLayout();
   avatarLayout->setContentsMargins(QMargins());
   avatarLayout->setSpacing(0);
   avatarLayout->addStretch();
   avatarLayout->addWidget(createAvatar(review.creator.name, review.creator.avatar, QSize(20, 20)));
   avatarLayout->addSpacing(5);
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

   const auto layout = new QHBoxLayout(this);
   layout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
   layout->setContentsMargins(QMargins());
   layout->setSpacing(20);
   layout->addLayout(avatarLayout);
   layout->addWidget(frame);
}

QLabel *CodeReviewComment::createHeadline(const QDateTime &dt, const QString &prefix)
{
   const auto days = dt.daysTo(QDateTime::currentDateTime());
   const auto whenText = days <= 30
       ? days != 0 ? QString::fromUtf8(" %1 days ago").arg(days) : QString::fromUtf8(" today")
       : QString(" on %1").arg(dt.date().toString(QLocale().dateFormat(QLocale::ShortFormat)));

   const auto label = prefix.isEmpty() ? new QLabel(whenText) : new QLabel(prefix + whenText);
   label->setToolTip(dt.toString(QLocale().dateTimeFormat(QLocale::ShortFormat)));

   return label;
}
