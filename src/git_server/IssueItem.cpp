#include <IssueItem.h>

#include <ButtonLink.hpp>

#include <QDir>
#include <QFile>
#include <QUrl>
#include <QGridLayout>
#include <QDesktopServices>
#include <QNetworkAccessManager>
#include <QStandardPaths>
#include <QNetworkRequest>
#include <QNetworkReply>

IssueItem::IssueItem(const ServerIssue &issueData, QWidget *parent)
   : QFrame(parent)
   , mManager(new QNetworkAccessManager())
   , mIssue(issueData)
   , mAvatar(new QLabel())
{
   setObjectName("IssueItem");

   /*
   const auto fileName
       = QString("%1/%2.png").arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation),
   mIssue.creator.name);

   if (!QFile(fileName).exists())
   {
      QNetworkRequest request;
      request.setUrl(mIssue.creator.avatar);
      const auto reply = mManager->get(request);
      connect(reply, &QNetworkReply::finished, this, &IssueButton::storeCreatorAvatar);
   }
   else
   {
      QPixmap img(fileName);
      img = img.scaled(25, 25);

      mCreator->setPixmap(img);
   }
   */

   const auto title = new ButtonLink(mIssue.title);
   title->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
   title->setWordWrap(true);
   title->setObjectName("IssueTitle");
   connect(title, &ButtonLink::clicked, [url = mIssue.url]() { QDesktopServices::openUrl(QUrl(url)); });

   const auto titleLayout = new QHBoxLayout();
   titleLayout->setContentsMargins(QMargins());
   titleLayout->setSpacing(0);
   titleLayout->addWidget(title);
   titleLayout->addStretch();

   const auto creationLayout = new QHBoxLayout();
   creationLayout->setContentsMargins(QMargins());
   creationLayout->setSpacing(0);
   creationLayout->addWidget(new QLabel(tr("Created by ")));
   const auto creator = new ButtonLink(QString("<b>%1</b>").arg(mIssue.creator.name));
   creator->setObjectName("CreatorLink");
   connect(creator, &ButtonLink::clicked, [url = mIssue.creator.url]() { QDesktopServices::openUrl(url); });

   creationLayout->addWidget(creator);

   const auto whenLabel
       = new QLabel(QString::fromUtf8(" %2 days ago").arg(mIssue.creation.daysTo(QDateTime::currentDateTime())));
   creationLayout->addWidget(whenLabel);
   creationLayout->addStretch();

   whenLabel->setToolTip(mIssue.creation.toString(Qt::SystemLocaleShortDate));

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

   auto row = 0;
   const auto layout = new QGridLayout(this);
   layout->setContentsMargins(0, 10, 0, 10);
   layout->setSpacing(5);
   layout->addLayout(titleLayout, row++, 1);
   layout->addLayout(creationLayout, row++, 1);

   if (!mIssue.assignees.isEmpty())
   {
      const auto assigneeLayout = new QHBoxLayout();
      assigneeLayout->setContentsMargins(QMargins());
      assigneeLayout->setSpacing(0);
      assigneeLayout->addWidget(new QLabel(tr("Assigned to ")));

      auto count = 0;
      const auto totalAssignees = mIssue.assignees.count();
      for (auto &assignee : mIssue.assignees)
      {
         const auto assigneLabel = new ButtonLink(QString("<b>%1</b>").arg(assignee.name));
         assigneLabel->setObjectName("CreatorLink");
         connect(assigneLabel, &ButtonLink::clicked, [url = assignee.url]() { QDesktopServices::openUrl(url); });
         assigneeLayout->addWidget(assigneLabel);

         if (count++ < totalAssignees - 1)
            assigneeLayout->addWidget(new QLabel(", "));
      }
      assigneeLayout->addStretch();

      layout->addLayout(assigneeLayout, row++, 1);
   }

   layout->addLayout(labelsLayout, row++, 1);

   layout->addWidget(mAvatar, 0, 0, row, 1);
}

void IssueItem::storeCreatorAvatar()
{
   const auto reply = qobject_cast<QNetworkReply *>(sender());
   const auto data = reply->readAll();

   QDir dir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
   if (!dir.exists())
      dir.mkpath(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));

   QString path = dir.absolutePath() + "/" + mIssue.creator.name + ".png";
   QFile file(path);
   if (file.open(QIODevice::WriteOnly))
   {
      file.write(data);
      file.close();

      QPixmap img(path);
      img = img.scaled(25, 25);

      mAvatar->setPixmap(img);
   }

   reply->deleteLater();
}
