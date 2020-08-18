#include <PrCommitsList.h>

#include <GitServerCache.h>
#include <GitHubRestApi.h>
#include <GitLabRestApi.h>
#include <CircularPixmap.h>
#include <ButtonLink.hpp>

#include <QApplication>
#include <QClipboard>
#include <QToolTip>
#include <QCursor>
#include <QLabel>
#include <QScrollArea>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QNetworkReply>
#include <QDir>
#include <QFile>
#include <QDesktopServices>

PrCommitsList::PrCommitsList(const QSharedPointer<GitServerCache> &gitServerCache, QWidget *parent)
   : QFrame(parent)
   , mGitServerCache(gitServerCache)
   , mManager(new QNetworkAccessManager())
{
}

void PrCommitsList::loadData(int number)
{
   connect(mGitServerCache.get(), &GitServerCache::prUpdated, this, &PrCommitsList::onCommitsReceived,
           Qt::UniqueConnection);

   mPrNumber = number;

   const auto pr = mGitServerCache->getPullRequest(number);

   mGitServerCache->getApi()->requestCommitsFromPR(pr);
}

void PrCommitsList::onCommitsReceived(const GitServer::PullRequest &pr)
{
   disconnect(mGitServerCache.get(), &GitServerCache::prUpdated, this, &PrCommitsList::onCommitsReceived);

   if (mPrNumber != pr.number)
      return;

   delete mScroll;

   mPrNumber = pr.number;

   const auto commitsLayout = new QVBoxLayout();
   commitsLayout->setContentsMargins(20, 20, 20, 20);
   commitsLayout->setAlignment(Qt::AlignTop);
   commitsLayout->setSpacing(30);

   const auto mIssuesFrame = new QFrame();
   mIssuesFrame->setObjectName("IssuesViewFrame");
   mIssuesFrame->setLayout(commitsLayout);

   mScroll = new QScrollArea();
   mScroll->setWidgetResizable(true);
   mScroll->setWidget(mIssuesFrame);

   delete layout();

   const auto aLayout = new QVBoxLayout(this);
   aLayout->setContentsMargins(QMargins());
   aLayout->setSpacing(0);
   aLayout->addWidget(mScroll);

   for (auto &commit : pr.commits)
   {
      const auto bubble = createBubbleForComment(commit);
      commitsLayout->addWidget(bubble);
   }

   commitsLayout->addStretch();
}

QFrame *PrCommitsList::createBubbleForComment(const GitServer::Commit &commit)
{
   const auto days = commit.authorCommittedTimestamp.daysTo(QDateTime::currentDateTime());
   const auto whenText = days <= 30
       ? days != 0 ? QString::fromUtf8(" %1 days ago").arg(days) : QString::fromUtf8(" today")
       : QString(" on %1").arg(
           commit.authorCommittedTimestamp.date().toString(QLocale().dateFormat(QLocale::ShortFormat)));

   const auto creator = new QLabel(QString("Committed by <b>%1</b> %2").arg(commit.author.name, whenText));

   const auto link = new ButtonLink(QString("<b>%1</b>").arg(commit.message.split("\n\n").constFirst()));
   connect(link, &ButtonLink::clicked, [url = commit.url]() { QDesktopServices::openUrl(url); });

   const auto frame = new QFrame();
   frame->setObjectName("IssueIntro");

   const auto layout = new QGridLayout(frame);
   layout->setAlignment(Qt::AlignVertical_Mask);
   layout->setContentsMargins(10, 10, 10, 10);
   layout->setHorizontalSpacing(10);
   layout->setVerticalSpacing(5);
   layout->addWidget(createAvatar(commit.author.name, commit.author.avatar), 0, 0, 2, 1);
   layout->addWidget(link, 0, 1);
   layout->addWidget(creator, 1, 1);
   layout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Fixed), 1, 2);

   const auto shaLink = new ButtonLink(commit.sha);
   connect(shaLink, &ButtonLink::clicked, this, [this, sha = commit.sha]() {
      QApplication::clipboard()->setText(sha);
      QToolTip::showText(QCursor::pos(), tr("Copied!"), this);
   });

   layout->addWidget(shaLink, 0, 3, 3, 1);

   if (commit.author.name != commit.commiter.name)
      layout->addWidget(createAvatar(commit.commiter.name, commit.commiter.avatar));

   return frame;
}

QLabel *PrCommitsList::createAvatar(const QString &userName, const QString &avatarUrl) const
{
   const auto fileName
       = QString("%1/%2").arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation), userName);
   const auto avatar = new CircularPixmap(QSize(30, 30));
   avatar->setObjectName("Avatar");
   avatar->setCentered(true);

   if (!QFile(fileName).exists())
   {
      QNetworkRequest request;
      request.setUrl(avatarUrl);
      const auto reply = mManager->get(request);
      connect(reply, &QNetworkReply::finished, this,
              [userName, this, avatar]() { storeCreatorAvatar(avatar, userName); });
   }
   else
   {
      QPixmap img(fileName);
      img = img.scaled(30, 30, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

      avatar->setPixmap(img);
   }

   return avatar;
}

void PrCommitsList::storeCreatorAvatar(QLabel *avatar, const QString &fileName) const
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
      img = img.scaled(50, 50, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

      avatar->setPixmap(img);
   }

   reply->deleteLater();
}
