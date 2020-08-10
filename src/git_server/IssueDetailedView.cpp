#include <IssueDetailedView.h>

#include <IRestApi.h>
#include <CreateIssueDlg.h>
#include <IssueItem.h>
#include <GitQlientSettings.h>
#include <GitConfig.h>
#include <GitHubRestApi.h>
#include <GitLabRestApi.h>

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>
#include <QStandardPaths>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>
#include <QDir>
#include <QNetworkAccessManager>

using namespace GitServer;

IssueDetailedView::IssueDetailedView(const QSharedPointer<GitBase> &git, Config config, QWidget *parent)
   : QFrame(parent)
   , mGit(git)
   , mConfig(config)
   , mManager(new QNetworkAccessManager())
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

   connect(mApi, &IRestApi::commentsReceived, this, &IssueDetailedView::onCommentReceived);

   const auto headerTitle = new QLabel(tr("Detailed View"));
   headerTitle->setObjectName("HeaderTitle");

   const auto headerFrame = new QFrame();
   headerFrame->setObjectName("IssuesHeaderFrame");
   const auto headerLayout = new QHBoxLayout(headerFrame);
   headerLayout->setContentsMargins(QMargins());
   headerLayout->setSpacing(0);
   headerLayout->addWidget(headerTitle);
   headerLayout->addStretch();

   mIssueDetailedView = new QFrame();

   mIssueDetailedViewLayout = new QHBoxLayout();
   mIssueDetailedViewLayout->setContentsMargins(QMargins());
   mIssueDetailedViewLayout->setSpacing(0);
   mIssueDetailedViewLayout->addWidget(mIssueDetailedView);

   const auto aLayout = new QHBoxLayout(mIssueDetailedView);
   aLayout->setContentsMargins(QMargins());
   aLayout->setSpacing(0);

   const auto issuesFrame = new QFrame();
   issuesFrame->setObjectName("IssuesViewFrame");
   mIssuesLayout = new QVBoxLayout(issuesFrame);
   mIssuesLayout->setContentsMargins(20, 20, 20, 20);
   mIssuesLayout->setAlignment(Qt::AlignTop);
   mIssuesLayout->setSpacing(30);

   aLayout->addWidget(issuesFrame);

   const auto footerFrame = new QFrame();
   footerFrame->setObjectName("IssuesFooterFrame");

   const auto issuesLayout = new QVBoxLayout(this);
   issuesLayout->setContentsMargins(QMargins());
   issuesLayout->setSpacing(0);
   issuesLayout->addWidget(headerFrame);
   issuesLayout->addLayout(mIssueDetailedViewLayout);
   issuesLayout->addWidget(footerFrame);

   const auto timer = new QTimer();
   // connect(timer, &QTimer::timeout, this, &IssueDetailedView::loadData);
   timer->start(300000);
}

void IssueDetailedView::loadData(const GitServer::Issue &issue)
{
   if (mLoaded && mIssue.number == issue.number)
      return;

   delete mIssueDetailedView;

   mIssueDetailedView = new QFrame();
   mIssueDetailedViewLayout->addWidget(mIssueDetailedView);

   const auto issuesFrame = new QFrame();
   issuesFrame->setObjectName("IssuesViewFrame");
   mIssuesLayout = new QVBoxLayout(issuesFrame);
   mIssuesLayout->setContentsMargins(20, 20, 20, 20);
   mIssuesLayout->setAlignment(Qt::AlignTop);
   mIssuesLayout->setSpacing(30);

   const auto scroll = new QScrollArea();
   scroll->setWidgetResizable(true);
   scroll->setWidget(issuesFrame);

   const auto aLayout = new QHBoxLayout(mIssueDetailedView);
   aLayout->setContentsMargins(QMargins());
   aLayout->setSpacing(0);
   aLayout->addWidget(scroll);

   mIssue = issue;

   const auto title = new QLabel(mIssue.title);
   title->setWordWrap(false);
   title->setObjectName("IssueViewTitle");

   const auto titleLayout = new QHBoxLayout();
   titleLayout->setContentsMargins(QMargins());
   titleLayout->setSpacing(0);
   titleLayout->addWidget(title);
   titleLayout->addStretch();

   const auto creationLayout = new QHBoxLayout();
   creationLayout->setContentsMargins(QMargins());
   creationLayout->setSpacing(0);
   creationLayout->addWidget(new QLabel(tr("Created by ")));
   const auto creator = new QLabel(QString("<b>%1</b>").arg(mIssue.creator.name));
   creator->setObjectName("CreatorLink");

   creationLayout->addWidget(creator);

   const auto days = mIssue.creation.daysTo(QDateTime::currentDateTime());
   const auto whenText = days <= 30 ? QString::fromUtf8(" %1 days ago").arg(days)
                                    : QString(" on %1").arg(mIssue.creation.date().toString(Qt::SystemLocaleShortDate));

   const auto whenLabel = new QLabel(whenText);
   whenLabel->setToolTip(mIssue.creation.toString(Qt::SystemLocaleShortDate));

   creationLayout->addWidget(whenLabel);
   creationLayout->addStretch();

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
      creationLayout->addStretch();
   }

   const auto labelsLayout = new QHBoxLayout();
   labelsLayout->setContentsMargins(QMargins());
   labelsLayout->setSpacing(10);

   QStringList labelsList;

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
      labelsLayout->addWidget(labelWidget);
   }

   const auto milestone = new QLabel(QString("%1").arg(mIssue.milestone.title));
   milestone->setObjectName("IssueLabel");
   labelsLayout->addWidget(milestone);
   labelsLayout->addStretch();

   const auto frame = new QFrame();
   frame->setObjectName("IssueIntro");
   const auto layout = new QVBoxLayout(frame);
   layout->setContentsMargins(10, 10, 10, 10);
   layout->setSpacing(5);
   layout->addLayout(titleLayout);
   layout->addLayout(creationLayout);
   layout->addLayout(labelsLayout);
   layout->addSpacing(20);

   const auto body = new QLabel(mIssue.body);
   body->setWordWrap(true);
   layout->addWidget(body);

   mIssuesLayout->addWidget(frame);

   mApi->requestComments(mIssue.number);
}

void IssueDetailedView::onCommentReceived(int issue, const QVector<GitServer::Comment> &comments)
{
   if (issue == mIssue.number)
   {
      for (auto &comment : comments)
      {
         const auto creationLayout = new QHBoxLayout();
         creationLayout->setContentsMargins(QMargins());
         creationLayout->setSpacing(0);
         creationLayout->addWidget(new QLabel(tr("Comment by ")));
         const auto creator = new QLabel(QString("<b>%1</b>").arg(comment.creator.name));
         creator->setObjectName("CreatorLink");

         creationLayout->addWidget(creator);

         const auto days = comment.creation.daysTo(QDateTime::currentDateTime());
         const auto whenText = days <= 30
             ? QString::fromUtf8(" %1 days ago").arg(days)
             : QString(" on %1").arg(comment.creation.date().toString(Qt::SystemLocaleShortDate));

         const auto whenLabel = new QLabel(whenText);
         whenLabel->setToolTip(comment.creation.toString(Qt::SystemLocaleShortDate));

         creationLayout->addWidget(whenLabel);
         creationLayout->addStretch();
         creationLayout->addWidget(new QLabel(comment.association));

         const auto body = new QLabel(comment.body);
         body->setWordWrap(true);

         const auto frame = new QFrame();
         frame->setObjectName("IssueIntro");

         const auto innerLayout = new QVBoxLayout(frame);
         innerLayout->setContentsMargins(10, 10, 10, 10);
         innerLayout->setSpacing(5);
         innerLayout->addLayout(creationLayout);
         innerLayout->addSpacing(20);
         innerLayout->addWidget(body);

         const auto fileName
             = QString("%1/%2.png")
                   .arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation), comment.creator.name);

         const auto avatar = new QLabel();
         avatar->setFixedSize(30, 30);
         avatar->setObjectName("Avatar");
         if (!QFile(fileName).exists())
         {
            QNetworkRequest request;
            request.setUrl(comment.creator.avatar);
            const auto reply = mManager->get(request);
            connect(reply, &QNetworkReply::finished, this,
                    [fileName = comment.creator.name, this, avatar]() { storeCreatorAvatar(avatar, fileName); });
         }
         else
         {
            QPixmap img(fileName);
            img = img.scaled(30, 30);

            avatar->setPixmap(img);
         }

         const auto layout = new QHBoxLayout();
         layout->setContentsMargins(QMargins());
         layout->setSpacing(30);
         layout->addSpacing(30);
         layout->addWidget(avatar);
         layout->addWidget(frame);

         mIssuesLayout->addLayout(layout);
      }
   }
}

void IssueDetailedView::storeCreatorAvatar(QLabel *avatar, const QString &fileName)
{
   const auto reply = qobject_cast<QNetworkReply *>(sender());
   const auto data = reply->readAll();

   QDir dir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
   if (!dir.exists())
      dir.mkpath(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));

   QString path = dir.absolutePath() + "/" + fileName + ".png";
   QFile file(path);
   if (file.open(QIODevice::WriteOnly))
   {
      file.write(data);
      file.close();

      QPixmap img(path);
      img = img.scaled(25, 25);

      avatar->setPixmap(img);
   }

   reply->deleteLater();
}
