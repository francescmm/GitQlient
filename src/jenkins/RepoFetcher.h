#pragma once

#include <IFetcher.h>
#include <JenkinsViewInfo.h>

namespace QJenkins
{

class RepoFetcher final : public IFetcher
{
   Q_OBJECT

signals:
   void signalViewsReceived(const QVector<JenkinsViewInfo> &views);

public:
   explicit RepoFetcher(const QString &user, const QString &token, const QString &url, QObject *parent = nullptr);

   void triggerFetch() override;

private:
   QString mUrl;
   void processData(const QJsonDocument &json) override;
};

}
