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
   , mCreator(new QLabel())
   , mTitle(new ButtonLink(mIssue.title))
   , mLabels(new QLabel(
         QString::fromUtf8("%1 days ago - %2")
             .arg(QString::number(mIssue.creation.daysTo(QDateTime::currentDateTime())), mIssue.labels.join(", "))))
   , mMilestone(new QLabel(mIssue.milestone.title))
{
   /*
   const auto fileName
       = QString("%1/%2.png").arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation), mIssue.creator.name);

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

   mLabels->setObjectName("IssueLabel");
   mLabels->setToolTip(mIssue.creation.toString(Qt::SystemLocaleShortDate));

   mMilestone->setObjectName("IssueLabel");

   const auto separator = new QFrame();
   separator->setObjectName("orangeHSeparator");

   const auto layout = new QGridLayout(this);
   layout->setContentsMargins(QMargins());
   layout->setSpacing(5);
   layout->addWidget(mCreator, 0, 0, 4, 1);
   layout->addWidget(mTitle, 0, 1);
   layout->addWidget(mLabels, 1, 1);
   layout->addWidget(mMilestone, 2, 1);
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

      mCreator->setPixmap(img);
   }

   reply->deleteLater();
}
