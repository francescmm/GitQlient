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

class ServerRestApi : public QObject
{
   Q_OBJECT

signals:

public:
   explicit ServerRestApi(QObject *parent = nullptr)
      : QObject(parent)
   {
   }

protected:
   QString mServerUrl;
   QString mRepoName;
   QString mRepoOwner;
   QString mUserName;
   QString mUserPassword;
   QNetworkAccessManager *mManager;

   virtual QUrl formatUrl(const QString endPoint) const final
   {
      QUrl url(mServerUrl + mRepoOwner + mRepoName + endPoint);
      url.setUserName(mUserName);
      url.setPassword(mUserPassword);
      return url;
   }
};

class GitHubRestApi : public ServerRestApi
{
   Q_OBJECT

signals:
   void signalLabelsReceived(const QVector<ServerLabel> &labels);
   void signalMilestonesReceived(const QVector<ServerMilestone> &milestones);
   void signalIssueCreated();
   void signalPullRequestCreated();

public:
   explicit GitHubRestApi(const QString &repoOwner, const QString repoName, QObject *parent = nullptr);

   void createIssue();
   void createPullRequest();

   void requestLabels();
   void getMilestones();

private:
   void onLabelsReceived(QNetworkReply *reply);
};
