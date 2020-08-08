#include <IssueButton.h>

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

IssueButton::IssueButton(const ServerIssue &issueData, QWidget *parent)
   : QFrame(parent)
   , mManager(new QNetworkAccessManager())
   , mIssue(issueData)
   , mAvatar(new QLabel())
   , mTitle(new ButtonLink(mIssue.title))
   , mMilestone(new QLabel(QString("%1").arg(mIssue.milestone.title)))

{
   const auto labelsLayout = new QHBoxLayout();
   labelsLayout->setContentsMargins(5, 0, 0, 0);
   labelsLayout->setSpacing(5);

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

   labelsLayout->addWidget(mMilestone);
   labelsLayout->addStretch();

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

   mTitle->setObjectName("IssueTitle");

   // mCreation->setObjectName("IssueLabel");

   mMilestone->setObjectName("IssueLabel");

   const auto separator = new QFrame();
   separator->setObjectName("orangeHSeparator");

   const auto creationLayout = new QHBoxLayout();
   creationLayout->setContentsMargins(5, 0, 0, 0);
   creationLayout->setSpacing(0);
   creationLayout->addWidget(new QLabel(tr("Created by ")));
   const auto creator = new ButtonLink(mIssue.creator.name);
   connect(creator, &ButtonLink::clicked, [url = mIssue.creator.url]() { QDesktopServices::openUrl(url); });

   creationLayout->addWidget(creator);

   const auto whenLabel
       = new QLabel(QString::fromUtf8(" %2 days ago").arg(mIssue.creation.daysTo(QDateTime::currentDateTime())));
   creationLayout->addWidget(whenLabel);
   creationLayout->addStretch();

   whenLabel->setToolTip(mIssue.creation.toString(Qt::SystemLocaleShortDate));

   const auto layout = new QGridLayout(this);
   layout->setContentsMargins(QMargins());
   layout->setSpacing(5);
   layout->addWidget(mAvatar, 0, 0, 4, 1);
   layout->addWidget(mTitle, 0, 1);
   layout->addLayout(creationLayout, 1, 1);
   layout->addLayout(labelsLayout, 2, 1);
   layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 3, 1);
   layout->addWidget(separator, 4, 0, 1, 2);

   /*
   QStringList assignees;
   for (auto &assignee : mIssue.assignees)
      assignees.append(assignee.name);

   layout->addWidget(new QLabel(assignees.join(", ")), 3, 0);
   */

   connect(mTitle, &ButtonLink::clicked, [url = mIssue.url]() { QDesktopServices::openUrl(QUrl(url)); });
}

void IssueButton::storeCreatorAvatar()
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
