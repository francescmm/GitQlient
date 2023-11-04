#include "GitQlientUpdater.h"

#include <GitQlientStyles.h>
#include <QLogger.h>

#include <QDesktopServices>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProgressDialog>
#include <QStandardPaths>
#include <QTimer>

using namespace QLogger;

GitQlientUpdater::GitQlientUpdater(QObject *parent)
   : QObject(parent)
   , mManager(new QNetworkAccessManager())
{
}

GitQlientUpdater::~GitQlientUpdater()
{
   delete mManager;
}

void GitQlientUpdater::checkNewGitQlientVersion()
{
   QNetworkRequest request;
   request.setRawHeader("User-Agent", "GitQlient");
   request.setRawHeader("X-Custom-User-Agent", "GitQlient");
   request.setRawHeader("Content-Type", "application/json");
   request.setUrl(QUrl("https://github.com/francescmm/ci-utils/releases/download/gq_update/updates.json"));
   request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);

   const auto reply = mManager->get(request);
   connect(reply, &QNetworkReply::finished, this, &GitQlientUpdater::processUpdateFile);
}

void GitQlientUpdater::showInfoMessage()
{
   QMessageBox msgBox(
       QMessageBox::Information, tr("New version of GitQlient!"),
       QString(tr("There is a new version of GitQlient available. Your current version is {%1} and the new "
                  "one is {%2}. You can read more about the new changes in the detailed description."))
           .arg(QString::fromUtf8(VER), mLatestGitQlient),
       QMessageBox::Ok | QMessageBox::Close, qobject_cast<QWidget *>(parent()));
   msgBox.setButtonText(QMessageBox::Ok, tr("Go to GitHub"));
   msgBox.setDetailedText(mChangeLog);
   msgBox.setStyleSheet(GitQlientStyles::getStyles());

   if (msgBox.exec() == QMessageBox::Ok)
   {
      QString url = QString::fromUtf8("https://github.com/francescmm/GitQlient/releases/tag/v%1").arg(mLatestGitQlient);
      QDesktopServices::openUrl(QUrl(url));
   }
}

void GitQlientUpdater::processUpdateFile()
{
   const auto reply = qobject_cast<QNetworkReply *>(sender());
   const auto data = reply->readAll();
   const auto jsonDoc = QJsonDocument::fromJson(data);

   if (jsonDoc.isNull())
   {
      QLog_Error("Ui", QString("Error when parsing Json. Current data:\n%1").arg(QString::fromUtf8(data)));
      return;
   }

   const auto json = jsonDoc.object();
   mLatestGitQlient = json["latest-version"].toString();
   const auto curVersion = QString("%1").arg(VER).split(".");

   if (curVersion.count() == 1)
      return;

   const auto newVersion = mLatestGitQlient.split(".");
   const auto nv = newVersion.at(0).toInt() * 10000 + newVersion.at(1).toInt() * 100 + newVersion.at(2).toInt();
   const auto cv = curVersion.at(0).toInt() * 10000 + curVersion.at(1).toInt() * 100 + curVersion.at(2).toInt();

   if (nv > cv)
   {
      emit newVersionAvailable();

      QTimer::singleShot(200, this, [this, changeLogUrl = json["changelog"].toString()] {
         QNetworkRequest request;
         request.setRawHeader("User-Agent", "GitQlient");
         request.setRawHeader("X-Custom-User-Agent", "GitQlient");
         request.setRawHeader("Content-Type", "application/json");
         request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);
         request.setUrl(QUrl(changeLogUrl));

         const auto reply = mManager->get(request);

         connect(reply, &QNetworkReply::finished, this, &GitQlientUpdater::processChangeLog);
      });
   }
}

void GitQlientUpdater::processChangeLog()
{
   const auto reply = qobject_cast<QNetworkReply *>(sender());
   mChangeLog = QString::fromUtf8(reply->readAll());
}
