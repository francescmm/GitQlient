#include "PluginsDownloader.h"

#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProgressDialog>
#include <QSettings>
#include <QStandardPaths>

PluginsDownloader::PluginsDownloader(QObject *parent)
   : QObject { parent }
{
}

void PluginsDownloader::checkAvailablePlugins()
{
   QNetworkRequest request;
   request.setRawHeader("User-Agent", "GitQlient");
   request.setRawHeader("X-Custom-User-Agent", "GitQlient");
   request.setRawHeader("Content-Type", "application/json");
   request.setUrl(QUrl("https://github.com/francescmm/ci-utils/releases/download/gq_update/updates.json"));
   request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);

   const auto reply = mManager->get(request);
   connect(reply, &QNetworkReply::finished, this, &PluginsDownloader::processPluginsFile);
}

void PluginsDownloader::processPluginsFile() { }

void PluginsDownloader::downloadPlugin(const QString &url)
{
   QNetworkRequest request;
   request.setRawHeader("User-Agent", "GitQlient");
   request.setRawHeader("X-Custom-User-Agent", "GitQlient");
   request.setRawHeader("Content-Type", "application/octet-stream");
   request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);
   request.setUrl(QUrl(url));

   const auto fileName = url.split("/").last();

   const auto reply = mManager->get(request);

   mDownloads[reply] = qMakePair(0U, fileName);

   connect(reply, &QNetworkReply::downloadProgress, this, &PluginsDownloader::onDownloadProgress);
   connect(reply, &QNetworkReply::finished, this, &PluginsDownloader::onDownloadFinished);
}

void PluginsDownloader::onDownloadProgress(qint64 read, qint64 total)
{
   if (mDownloadLog == nullptr)
   {
      mTotal = total;

      mDownloadLog
          = new QProgressDialog(tr("Downloading..."), tr("Close"), 0, total, qobject_cast<QWidget *>(parent()));
      mDownloadLog->setAttribute(Qt::WA_DeleteOnClose);
      mDownloadLog->setAutoClose(false);
      mDownloadLog->setAutoReset(false);
      mDownloadLog->setMaximum(total);
      mDownloadLog->setCancelButton(nullptr);
      mDownloadLog->setWindowFlag(Qt::FramelessWindowHint);

      connect(mDownloadLog, &QProgressDialog::destroyed, this, [this]() { mDownloadLog = nullptr; });
   }
   else if (mDownloads.value(qobject_cast<QNetworkReply *>(sender())).first == 0U)
      mTotal += total;

   mDownloadLog->setMaximum(mTotal);
   mDownloadLog->setValue(read);
   mDownloadLog->show();
}

void PluginsDownloader::onDownloadFinished()
{
   const auto reply = qobject_cast<QNetworkReply *>(sender());
   const auto fileName = mDownloads.value(qobject_cast<QNetworkReply *>(sender())).second;
   const auto b = reply->readAll();
   const auto folder
       = QSettings()
             .value("PluginsFolder", QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).constFirst())
             .toString();

   if (QFile file(QString("%1/%2").arg(folder, fileName)); file.open(QIODevice::WriteOnly))
   {
      QDataStream out(&file);
      out << b;

      file.close();
   }

   mDownloads.remove(reply);

   if (mDownloads.isEmpty())
   {
      mDownloadLog->close();
      mDownloadLog = nullptr;
   }

   reply->deleteLater();
}
