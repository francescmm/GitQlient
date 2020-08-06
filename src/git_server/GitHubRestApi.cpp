#include "GitHubRestApi.h"
#include <ServerIssue.h>

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>

#include <QLogger.h>

using namespace QLogger;

GitHubRestApi::GitHubRestApi(QString repoOwner, QString repoName, const ServerAuthentication &auth, QObject *parent)
   : IRestApi(auth, parent)
{
   if (!repoOwner.endsWith("/"))
      repoOwner.append("/");

   if (!repoOwner.startsWith("/"))
      repoOwner.prepend("/");

   if (repoName.endsWith("/"))
      repoName = repoName.left(repoName.size() - 1);

   mRepoEndpoint = QString("/repos") + repoOwner + repoName;
}

void GitHubRestApi::testConnection()
{
   auto request = createRequest("/user/repos");

   const auto reply = mManager->get(request);

   connect(reply, &QNetworkReply::finished, this, [this]() {
      const auto reply = qobject_cast<QNetworkReply *>(sender());
      QString errorStr;
      const auto tmpDoc = validateData(reply, errorStr);

      if (!tmpDoc.isEmpty())
         emit connectionTested();
      else
         emit errorOccurred(errorStr);
   });
}

void GitHubRestApi::createIssue(const ServerIssue &issue)
{
   QJsonDocument doc(issue.toJson());
   const auto data = doc.toJson(QJsonDocument::Compact);

   auto request = createRequest(mRepoEndpoint + "/issues");
   request.setRawHeader("Content-Length", QByteArray::number(data.size()));
   const auto reply = mManager->post(request, data);

   connect(reply, &QNetworkReply::finished, this, &GitHubRestApi::onIssueCreated);
}

void GitHubRestApi::updateIssue(int issueNumber, const ServerIssue &issue)
{
   QJsonDocument doc(issue.toJson());
   const auto data = doc.toJson(QJsonDocument::Compact);

   auto request = createRequest(QString(mRepoEndpoint + "/issues/%1").arg(issueNumber));
   request.setRawHeader("Content-Length", QByteArray::number(data.size()));
   const auto reply = mManager->post(request, data);

   connect(reply, &QNetworkReply::finished, this, [this]() {
      const auto reply = qobject_cast<QNetworkReply *>(sender());
      QString errorStr;
      const auto tmpDoc = validateData(reply, errorStr);

      if (!tmpDoc.isEmpty())
         emit issueUpdated();
      else
         emit errorOccurred(errorStr);
   });
}

void GitHubRestApi::createPullRequest(const ServerPullRequest &pullRequest)
{
   QJsonDocument doc(pullRequest.toJson());
   const auto data = doc.toJson(QJsonDocument::Compact);

   auto request = createRequest(mRepoEndpoint + "/pulls");
   request.setRawHeader("Content-Length", QByteArray::number(data.size()));

   const auto reply = mManager->post(request, data);
   connect(reply, &QNetworkReply::finished, this, &GitHubRestApi::onPullRequestCreated);
}

void GitHubRestApi::requestLabels()
{
   const auto reply = mManager->get(createRequest(mRepoEndpoint + "/labels"));

   connect(reply, &QNetworkReply::finished, this, &GitHubRestApi::onLabelsReceived);
}

void GitHubRestApi::requestMilestones()
{
   const auto reply = mManager->get(createRequest(mRepoEndpoint + "/milestones"));

   connect(reply, &QNetworkReply::finished, this, &GitHubRestApi::onMilestonesReceived);
}

void GitHubRestApi::requestPullRequestsState()
{
   QTimer::singleShot(1500, this, [this]() {
      const auto reply = mManager->get(createRequest(mRepoEndpoint + "/pulls"));
      connect(reply, &QNetworkReply::finished, this, &GitHubRestApi::processPullRequets);
   });
}

void GitHubRestApi::mergePullRequest(int number, const QByteArray &data)
{
   const auto reply = mManager->put(createRequest(mRepoEndpoint + QString("/pulls/%1/merge").arg(number)), data);

   connect(reply, &QNetworkReply::finished, this, &GitHubRestApi::onPullRequestMerged);
}

QNetworkRequest GitHubRestApi::createRequest(const QString &page) const
{
   QNetworkRequest request;
   request.setUrl(QString(mAuth.endpointUrl + page));
   request.setRawHeader("User-Agent", "GitQlient");
   request.setRawHeader("X-Custom-User-Agent", "GitQlient");
   request.setRawHeader("Content-Type", "application/json");
   request.setRawHeader("Accept", "application/vnd.github.v3+json");
   request.setRawHeader(
       QByteArray("Authorization"),
       QByteArray("Basic ")
           + QByteArray(QString(QStringLiteral("%1:%2")).arg(mAuth.userName).arg(mAuth.userPass).toLocal8Bit())
                 .toBase64());

   return request;
}

void GitHubRestApi::onLabelsReceived()
{
   const auto reply = qobject_cast<QNetworkReply *>(sender());
   QString errorStr;
   const auto tmpDoc = validateData(reply, errorStr);

   if (!tmpDoc.isEmpty())
   {
      const auto doc = tmpDoc;

      QVector<ServerLabel> labels;
      const auto labelsArray = doc.array();

      for (auto label : labelsArray)
      {
         const auto jobObject = label.toObject();
         ServerLabel sLabel { jobObject[QStringLiteral("id")].toInt(),
                              jobObject[QStringLiteral("node_id")].toString(),
                              jobObject[QStringLiteral("url")].toString(),
                              jobObject[QStringLiteral("name")].toString(),
                              jobObject[QStringLiteral("description")].toString(),
                              jobObject[QStringLiteral("color")].toString(),
                              jobObject[QStringLiteral("default")].toBool() };

         labels.append(std::move(sLabel));
      }

      emit labelsReceived(labels);
   }
   else
      emit errorOccurred(errorStr);
}

void GitHubRestApi::onMilestonesReceived()
{
   const auto reply = qobject_cast<QNetworkReply *>(sender());
   QString errorStr;
   const auto tmpDoc = validateData(reply, errorStr);

   if (!tmpDoc.isEmpty())
   {
      const auto doc = tmpDoc;
      QVector<ServerMilestone> milestones;
      const auto labelsArray = doc.array();

      for (auto label : labelsArray)
      {
         const auto jobObject = label.toObject();
         ServerMilestone sMilestone { jobObject[QStringLiteral("id")].toInt(),
                                      jobObject[QStringLiteral("number")].toInt(),
                                      jobObject[QStringLiteral("node_id")].toString(),
                                      jobObject[QStringLiteral("title")].toString(),
                                      jobObject[QStringLiteral("description")].toString(),
                                      jobObject[QStringLiteral("state")].toString() == "open" };

         milestones.append(std::move(sMilestone));
      }

      emit milestonesReceived(milestones);
   }
   else
      emit errorOccurred(errorStr);
}

void GitHubRestApi::onIssueCreated()
{
   const auto reply = qobject_cast<QNetworkReply *>(sender());
   QString errorStr;
   const auto tmpDoc = validateData(reply, errorStr);

   if (!tmpDoc.isEmpty())
   {
      const auto doc = tmpDoc;
      const auto issue = doc.object();
      const auto url = issue[QStringLiteral("html_url")].toString();

      emit issueCreated(url);
   }
   else
      emit errorOccurred(errorStr);
}

void GitHubRestApi::onPullRequestCreated()
{
   const auto reply = qobject_cast<QNetworkReply *>(sender());
   QString errorStr;
   const auto tmpDoc = validateData(reply, errorStr);

   if (!tmpDoc.isEmpty())
   {
      const auto doc = tmpDoc;
      const auto issue = doc.object();
      const auto url = issue[QStringLiteral("html_url")].toString();

      emit pullRequestCreated(url);
   }
   else
      emit errorOccurred(errorStr);
}

void GitHubRestApi::processPullRequets()
{
   const auto reply = qobject_cast<QNetworkReply *>(sender());
   QString errorStr;
   const auto tmpDoc = validateData(reply, errorStr);

   if (!tmpDoc.isEmpty())
   {
      const auto doc = tmpDoc;
      const auto prs = doc.array();

      mPulls.clear();

      ServerPullRequest prInfo;

      for (const auto &pr : prs)
      {
         prInfo.id = pr["number"].toInt();
         prInfo.title = pr["title"].toString();
         prInfo.body = pr["body"].toString().toUtf8();
         prInfo.head = pr["head"].toObject()["ref"].toString();
         prInfo.base = pr["base"].toObject()["ref"].toString();
         prInfo.isOpen = pr["state"].toString() == "open";
         prInfo.draft = pr["draft"].toBool();
         // prInfo.details;
         prInfo.url = pr["html_url"].toString();

         const auto headObj = pr["head"].toObject();

         prInfo.state.sha = headObj["sha"].toString();

         mPulls.insert(prInfo.state.sha, prInfo);

         auto request = createRequest(mRepoEndpoint + QString("/commits/%1/status").arg(prInfo.state.sha));
         const auto reply = mManager->get(request);
         connect(reply, &QNetworkReply::finished, this, &GitHubRestApi::onPullRequestStatusReceived);

         ++mPrRequested;
      }
   }
   else
      emit errorOccurred(errorStr);
}

void GitHubRestApi::onPullRequestStatusReceived()
{
   const auto reply = qobject_cast<QNetworkReply *>(sender());
   QString errorStr;
   const auto tmpDoc = validateData(reply, errorStr);

   if (!tmpDoc.isEmpty())
   {
      auto sha = reply->url().toString();
      sha.remove("/status");
      sha = sha.mid(sha.lastIndexOf("/") + 1);

      const auto obj = tmpDoc.object();

      mPulls[sha].state.state = obj["state"].toString();

      mPulls[sha].state.eState = mPulls[sha].state.state == "success"
          ? ServerPullRequest::HeadState::State::Success
          : mPulls[sha].state.state == "failure" ? ServerPullRequest::HeadState::State::Failure
                                                 : ServerPullRequest::HeadState::State::Pending;

      const auto statuses = obj["statuses"].toArray();

      for (auto status : statuses)
      {
         auto statusStr = status["state"].toString();

         if (statusStr == "ok")
            statusStr = "success";
         else if (statusStr == "error")
            statusStr = "failure";

         ServerPullRequest::HeadState::Check check { status["description"].toString(), statusStr,
                                                     status["target_url"].toString(), status["context"].toString() };

         mPulls[sha].state.checks.append(check);
      }

      --mPrRequested;

      if (mPrRequested == 0)
         emit pullRequestsReceived(mPulls);
   }
   else
      emit errorOccurred(errorStr);
}

void GitHubRestApi::onPullRequestMerged()
{
   const auto reply = qobject_cast<QNetworkReply *>(sender());
   QString errorStr;
   const auto tmpDoc = validateData(reply, errorStr);

   if (!tmpDoc.isEmpty())
      emit pullRequestMerged();
   else
      emit errorOccurred(errorStr);
}
