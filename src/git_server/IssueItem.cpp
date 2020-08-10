#include <IssueItem.h>

#include <ButtonLink.hpp>
#include <Issue.h>

#include <QDir>
#include <QFile>
#include <QUrl>
#include <QGridLayout>
#include <QDesktopServices>
#include <QNetworkAccessManager>
#include <QStandardPaths>
#include <QNetworkRequest>
#include <QNetworkReply>

using namespace GitServer;

IssueItem::IssueItem(const Issue &issueData, QWidget *parent)
   : QFrame(parent)
   , mManager(new QNetworkAccessManager())
   , mAvatar(new QLabel())
{
   setObjectName("IssueItem");

   /*
   const auto fileName
       = QString("%1/%2.png").arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation),
   issueData.creator.name);

   if (!QFile(fileName).exists())
   {
      QNetworkRequest request;
      request.setUrl(issueData.creator.avatar);
      const auto reply = mManager->get(request);
      connect(reply, &QNetworkReply::finished, this, [fileName = issueData.creator.name, this]() {
   storeCreatorAvatar(fileName); });
   }
   else
   {
      QPixmap img(fileName);
      img = img.scaled(25, 25);

      mCreator->setPixmap(img);
   }
   */

   const auto title = new ButtonLink(issueData.title);
   title->setWordWrap(false);
   title->setObjectName("IssueTitle");
   connect(title, &ButtonLink::clicked, [url = issueData.url]() { QDesktopServices::openUrl(QUrl(url)); });

   const auto titleLayout = new QHBoxLayout();
   titleLayout->setContentsMargins(QMargins());
   titleLayout->setSpacing(0);
   titleLayout->addWidget(title);
   titleLayout->addStretch();

   const auto creationLayout = new QHBoxLayout();
   creationLayout->setContentsMargins(QMargins());
   creationLayout->setSpacing(0);
   creationLayout->addWidget(new QLabel(tr("Created by ")));
   const auto creator = new ButtonLink(QString("<b>%1</b>").arg(issueData.creator.name));
   creator->setObjectName("CreatorLink");
   connect(creator, &ButtonLink::clicked, [this, id = issueData.number]() { emit selected(id); });

   creationLayout->addWidget(creator);

   const auto whenLabel
       = new QLabel(QString::fromUtf8(" %2 days ago").arg(issueData.creation.daysTo(QDateTime::currentDateTime())));
   creationLayout->addWidget(whenLabel);
   creationLayout->addStretch();

   whenLabel->setToolTip(issueData.creation.toString(Qt::SystemLocaleShortDate));

   const auto labelsLayout = new QHBoxLayout();
   labelsLayout->setContentsMargins(QMargins());
   labelsLayout->setSpacing(10);

   QStringList labelsList;

   for (auto &label : issueData.labels)
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

   const auto milestone = new QLabel(QString("%1").arg(issueData.milestone.title));
   milestone->setObjectName("IssueLabel");
   labelsLayout->addWidget(milestone);
   labelsLayout->addStretch();

   auto row = 0;
   const auto layout = new QGridLayout(this);
   layout->setContentsMargins(0, 10, 0, 10);
   layout->setSpacing(5);
   layout->addLayout(titleLayout, row++, 1);
   layout->addLayout(creationLayout, row++, 1);

   if (!issueData.assignees.isEmpty())
   {
      const auto assigneeLayout = new QHBoxLayout();
      assigneeLayout->setContentsMargins(QMargins());
      assigneeLayout->setSpacing(0);
      assigneeLayout->addWidget(new QLabel(tr("Assigned to ")));

      auto count = 0;
      const auto totalAssignees = issueData.assignees.count();
      for (auto &assignee : issueData.assignees)
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

void IssueItem::storeCreatorAvatar(const QString &fileName)
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

      mAvatar->setPixmap(img);
   }

   reply->deleteLater();
}
