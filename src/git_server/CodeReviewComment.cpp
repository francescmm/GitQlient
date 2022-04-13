#include <CodeReviewComment.h>

#include <AvatarHelper.h>
#include <Comment.h>
#include <previewpage.h>

#include <QLabel>
#include <QLocale>
#include <QSettings>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWebChannel>
#include <QWebEngineView>

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

   QPointer<QWebEngineView> body = new QWebEngineView();
   const auto page = new PreviewPage(this);
   body->setPage(page);

   const auto channel = new QWebChannel(this);
   channel->registerObject(QStringLiteral("content"), &m_content);
   page->setWebChannel(channel);

   body->setUrl(QUrl(QString("qrc:/resources/index_%1.html").arg(QSettings().value("colorSchema", "dark").toString())));
   body->setFixedHeight(20);

   connect(page, &PreviewPage::contentsSizeChanged, this, [body](const QSizeF size) {
      if (body)
         body->setFixedHeight(size.height());
   });

   m_content.setText(review.body);

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
   const auto whenText = days <= 30 ? days != 0 ? tr(" %1 days ago").arg(days) : tr(" today")
                                    : tr(" on %1").arg(dt.date().toString(QLocale().dateFormat(QLocale::ShortFormat)));

   const auto label = prefix.isEmpty() ? new QLabel(whenText) : new QLabel(prefix + whenText);
   label->setToolTip(dt.toString(QLocale().dateTimeFormat(QLocale::ShortFormat)));

   return label;
}
