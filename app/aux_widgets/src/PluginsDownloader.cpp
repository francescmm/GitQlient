#include "PluginsDownloader.h"

#include <QLogger.h>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProgressDialog>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>

PluginsDownloader::PluginsDownloader(QObject *parent)
   : QObject { parent }
   , mManager(new QNetworkAccessManager())
{
   qRegisterMetaType<PluginInfo>("PluginInfo");
   qRegisterMetaType<QVector<PluginInfo>>("QVector<PluginInfo>");
}

void PluginsDownloader::checkAvailablePlugins()
{
   QNetworkRequest request;
   request.setRawHeader("User-Agent", "GitQlient");
   request.setRawHeader("X-Custom-User-Agent", "GitQlient");
   request.setRawHeader("Content-Type", "application/json");
   request.setUrl(QUrl("https://github.com/francescmm/ci-utils/releases/download/gq_update/plugins.json"));
   request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);

   const auto reply = mManager->get(request);
   connect(reply, &QNetworkReply::finished, this, &PluginsDownloader::processPluginsFile);
}

void PluginsDownloader::processPluginsFile()
{
   const auto reply = qobject_cast<QNetworkReply *>(sender());
   const auto data = reply->readAll();
   const auto jsonDoc = QJsonDocument::fromJson(data);

   if (jsonDoc.isNull())
   {
      QLog_Error("Ui", QString("Error when parsing Json. Current data:\n%1").arg(QString::fromUtf8(data)));
      return;
   }

   QVector<PluginInfo> pluginsInfo;

   const auto plugins = jsonDoc.object()["plugins"].toArray();
   for (const auto &plugin : plugins)
   {
      const auto jsonObject = plugin.toObject();
      auto platform = QString("linux");

#if defined(Q_OS_WIN)
      platform = QString("windows");
#elif defined(Q_OS_MACOS)
      platform = QString("macos");
#endif
      if (const auto jsonUrl = QStringLiteral("%1-url").arg(platform); jsonObject.contains(jsonUrl))
      {
         PluginInfo pluginInfo {
            jsonObject["name"].toString(), jsonObject["version"].toString(), jsonObject[jsonUrl].toString(), {}
         };

         if (const auto dependenciesStr = QStringLiteral("dependencies"); jsonObject.contains(dependenciesStr))
         {
            const auto dependencies = jsonObject[dependenciesStr].toArray();
            for (const auto &dependency : dependencies)
               pluginInfo.dependencies.append(dependency.toString());
         }

         pluginsInfo.append(std::move(pluginInfo));
      }
   }

   emit availablePlugins(pluginsInfo);
}

void PluginsDownloader::downloadPlugin(const QString &url)
{
   QNetworkRequest request;
   request.setRawHeader("User-Agent", "GitQlient");
   request.setRawHeader("X-Custom-User-Agent", "GitQlient");
   request.setRawHeader("Content-Type", "application/octet-stream");
   request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);
   request.setUrl(QUrl(url));

   const auto fileName = url.split("/").last();

   if (mDownloadLog == nullptr)
   {
      mDownloadLog = new QProgressDialog(tr("Downloading..."), tr("Close"), 0, 100, qobject_cast<QWidget *>(parent()));
      mDownloadLog->setAttribute(Qt::WA_DeleteOnClose);
      mDownloadLog->setAutoClose(false);
      mDownloadLog->setAutoReset(false);
      mDownloadLog->setCancelButton(nullptr);
      mDownloadLog->setWindowFlag(Qt::FramelessWindowHint);
      mDownloadLog->show();

      connect(mDownloadLog, &QProgressDialog::destroyed, this, [this]() { mDownloadLog = nullptr; });
   }
   mDownloadLog->setValue(0);

   const auto reply = mManager->get(request);

   mDownloads[reply] = qMakePair(0U, fileName);

   connect(reply, &QNetworkReply::downloadProgress, this, &PluginsDownloader::onDownloadProgress);
   connect(reply, &QNetworkReply::finished, this, &PluginsDownloader::onDownloadFinished);
}

void PluginsDownloader::onDownloadProgress(qint64 read, qint64 total)
{
   if (mDownloadLog)
   {
      if (mDownloads.value(qobject_cast<QNetworkReply *>(sender())).first == 0U)
         mTotal += total;

      mDownloadLog->setMaximum(mTotal);
      mDownloadLog->setValue(read);
   }
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
      file.write(b);
      file.close();
   }

   mDownloads.remove(reply);

   if (mDownloads.isEmpty())
   {
      mDownloadLog->close();
      mDownloadLog = nullptr;
   }

   reply->deleteLater();

   emit pluginStored();
}
