#pragma once

#include <QObject>
#include <QUrl>

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

struct ServerAuthentication
{
   QString userName;
   QString userPass;
};

class QJsonDocument;
class QNetworkAccessManager;
class QNetworkReply;

class GitHubRestApi : public QObject
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
   QString mServerUrl;
   QString mRepoName;
   QString mRepoOwner;
   ServerAuthentication mAuth;
   QNetworkAccessManager *mManager;

   QUrl formatUrl(const QString endPoint) const;

   std::optional<QJsonDocument> validateData(QNetworkReply *reply);
   void onLabelsReceived(QNetworkReply *reply);
};
