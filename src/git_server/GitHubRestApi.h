#pragma once

#include <QObject>
#include <QUrl>
#include <GitQlientSettings.h>

struct ServerIssue
{
   QString title;
   QString body;
   QString assignee;
   int milestone;
   QStringList labels;
   QStringList assignees;
};

struct ServerPullRequest
{
   QString title;
   QString head;
   QString base;
   QString body;
   bool isOpen;
   bool maintainerCanModify;
   bool draft;
};

struct ServerLabel
{
   int id;
   QString nodeId;
   QString url;
   QString name;
   QString description;
   QString colorHex;
   bool isDefault;
};

struct ServerMilestone
{
   int id;
   int number;
   QString nodeId;
   QString title;
   QString description;
   bool isOpen;
};

class QNetworkAccessManager;
class QNetworkReply;

struct ServerAuthentication
{
   QString userName;
   QString userPass;
};

class ServerRestApi : public QObject
{
   Q_OBJECT

public:
   explicit ServerRestApi(QObject *parent = nullptr)
      : QObject(parent)
   {
   }

protected:
   QString mServerUrl;
   QString mRepoName;
   QString mRepoOwner;
   ServerAuthentication mAuth;
   QNetworkAccessManager *mManager;

   virtual QUrl formatUrl(const QString endPoint) const final
   {
      auto tmpUrl = mServerUrl + mRepoOwner + mRepoName + endPoint;
      if (tmpUrl.endsWith("/"))
         tmpUrl = tmpUrl.left(tmpUrl.size() - 1);

      QUrl url(tmpUrl);
      url.setUserName(mAuth.userName);
      url.setPassword(mAuth.userPass);
      return url;
   }
};

class QJsonDocument;

class GitHubRestApi : public ServerRestApi
{
   Q_OBJECT

signals:
   void signalConnectionSuccessful();
   void signalLabelsReceived(const QVector<ServerLabel> &labels);
   void signalMilestonesReceived(const QVector<ServerMilestone> &milestones);
   void signalIssueCreated();
   void signalPullRequestCreated();

public:
   explicit GitHubRestApi(const QString &repoOwner, const QString repoName, const ServerAuthentication &auth,
                          QObject *parent = nullptr);

   void testConnection();

   void createIssue();
   void createPullRequest();

   void requestLabels();
   void getMilestones();

private:
   void onLabelsReceived(QNetworkReply *reply);

   std::optional<QJsonDocument> validateData(QNetworkReply *reply);
};
