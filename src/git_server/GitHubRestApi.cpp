#include "GitHubRestApi.h"
#include <Issue.h>

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>

#include <QLogger.h>

using namespace QLogger;
using namespace GitServer;

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

void GitHubRestApi::createIssue(const Issue &issue)
{
   QJsonDocument doc(issue.toJson());
   const auto data = doc.toJson(QJsonDocument::Compact);

   auto request = createRequest(mRepoEndpoint + "/issues");
   request.setRawHeader("Content-Length", QByteArray::number(data.size()));
   const auto reply = mManager->post(request, data);

   connect(reply, &QNetworkReply::finished, this, &GitHubRestApi::onIssueCreated);
}

void GitHubRestApi::updateIssue(int issueNumber, const Issue &issue)
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

void GitHubRestApi::createPullRequest(const PullRequest &pullRequest)
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

void GitHubRestApi::requestIssues()
{
   const auto reply = mManager->get(createRequest(mRepoEndpoint + "/issues"));

   connect(reply, &QNetworkReply::finished, this, &GitHubRestApi::onIssuesReceived);
}

void GitHubRestApi::requestPullRequests()
{
   const auto reply = mManager->get(createRequest(mRepoEndpoint + "/pulls"));

   connect(reply, &QNetworkReply::finished, this, &GitHubRestApi::onIssuesReceived);
}

void GitHubRestApi::requestPullRequestsState()
{
   const auto reply = mManager->get(createRequest(mRepoEndpoint + "/pulls"));
   connect(reply, &QNetworkReply::finished, this, &GitHubRestApi::processPullRequets);
}

void GitHubRestApi::mergePullRequest(int number, const QByteArray &data)
{
   const auto reply = mManager->put(createRequest(mRepoEndpoint + QString("/pulls/%1/merge").arg(number)), data);

   connect(reply, &QNetworkReply::finished, this, &GitHubRestApi::onPullRequestMerged);
}

void GitHubRestApi::requestComments(const Issue &issue)
{
   const auto reply = mManager->get(createRequest(mRepoEndpoint + QString("/issues/%1/comments").arg(issue.number)));

   connect(reply, &QNetworkReply::finished, this, [this, issue]() { onCommentsReceived(issue); });
}

void GitHubRestApi::requestReviews(const PullRequest &pr)
{
   const auto reply = mManager->get(createRequest(mRepoEndpoint + QString("/pulls/%1/reviews").arg(pr.number)));

   connect(reply, &QNetworkReply::finished, this, [this, pr]() { onReviewsReceived(pr); });
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
      QVector<Label> labels;
      const auto labelsArray = tmpDoc.array();

      for (auto label : labelsArray)
      {
         const auto jobObject = label.toObject();
         Label sLabel { jobObject[QStringLiteral("id")].toInt(),
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
      QVector<Milestone> milestones;
      const auto labelsArray = tmpDoc.array();

      for (auto label : labelsArray)
      {
         const auto jobObject = label.toObject();
         Milestone sMilestone { jobObject[QStringLiteral("id")].toInt(),
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
      const auto issue = tmpDoc.object();
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
      const auto issue = tmpDoc.object();
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
      const auto prs = tmpDoc.array();

      mPulls.clear();

      PullRequest prInfo;

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
          ? PullRequest::HeadState::State::Success
          : mPulls[sha].state.state == "failure" ? PullRequest::HeadState::State::Failure
                                                 : PullRequest::HeadState::State::Pending;

      const auto statuses = obj["statuses"].toArray();

      for (auto status : statuses)
      {
         auto statusStr = status["state"].toString();

         if (statusStr == "ok")
            statusStr = "success";
         else if (statusStr == "error")
            statusStr = "failure";

         PullRequest::HeadState::Check check { status["description"].toString(), statusStr,
                                               status["target_url"].toString(), status["context"].toString() };

         mPulls[sha].state.checks.append(std::move(check));
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

void GitHubRestApi::onIssuesReceived()
{
   const auto reply = qobject_cast<QNetworkReply *>(sender());
   const auto url = reply->url();
   QString errorStr;
   const auto tmpDoc = validateData(reply, errorStr);

   if (!tmpDoc.isEmpty())
   {
      QVector<Issue> issues;
      const auto issuesArray = tmpDoc.array();

      for (const auto &issueData : issuesArray)
      {
         auto issue = url.url().endsWith("/pulls") ? PullRequest() : Issue();
         issue.number = issueData["number"].toInt();
         issue.title = issueData["title"].toString();
         issue.body = issueData["body"].toString().toUtf8();
         issue.url = issueData["html_url"].toString();
         issue.creation = issueData["created_at"].toVariant().toDateTime();

         issue.creator
             = { issueData["user"].toObject()["id"].toInt(), issueData["user"].toObject()["login"].toString(),
                 issueData["user"].toObject()["avatar_url"].toString(),
                 issueData["user"].toObject()["html_url"].toString(), issueData["user"].toObject()["type"].toString() };

         const auto labels = issueData["labels"].toArray();

         for (auto label : labels)
         {
            issue.labels.append({ label["id"].toInt(), label["node_id"].toString(), label["url"].toString(),
                                  label["name"].toString(), label["description"].toString(), label["color"].toString(),
                                  label["default"].toBool() });
         }

         const auto assignees = issueData["assignees"].toArray();

         for (auto assignee : assignees)
         {
            GitServer::User sAssignee;
            sAssignee.id = assignee["id"].toInt();
            sAssignee.url = assignee["html_url"].toString();
            sAssignee.name = assignee["login"].toString();
            sAssignee.avatar = assignee["avatar_url"].toString();

            issue.assignees.append(sAssignee);
         }

         Milestone sMilestone { issueData["milestone"].toObject()[QStringLiteral("id")].toInt(),
                                issueData["milestone"].toObject()[QStringLiteral("number")].toInt(),
                                issueData["milestone"].toObject()[QStringLiteral("node_id")].toString(),
                                issueData["milestone"].toObject()[QStringLiteral("title")].toString(),
                                issueData["milestone"].toObject()[QStringLiteral("description")].toString(),
                                issueData["milestone"].toObject()[QStringLiteral("state")].toString() == "open" };

         issue.milestone = sMilestone;

         issues.append(std::move(issue));
      }

      if (!issues.isEmpty())
         emit issuesReceived(issues);
   }
}

void GitHubRestApi::onCommentsReceived(Issue issue)
{
   const auto reply = qobject_cast<QNetworkReply *>(sender());
   const auto url = reply->url();
   QString errorStr;
   const auto tmpDoc = validateData(reply, errorStr);

   if (!tmpDoc.isEmpty())
   {
      QVector<Comment> comments;
      const auto commentsArray = tmpDoc.array();

      for (const auto &commentData : commentsArray)
      {
         Comment c;
         c.id = commentData["id"].toInt();
         c.body = commentData["body"].toString();
         c.creation = commentData["created_at"].toVariant().toDateTime();
         c.association = commentData["author_association"].toString();

         GitServer::User sAssignee;
         sAssignee.id = commentData["user"].toObject()["id"].toInt();
         sAssignee.url = commentData["user"].toObject()["html_url"].toString();
         sAssignee.name = commentData["user"].toObject()["login"].toString();
         sAssignee.avatar = commentData["user"].toObject()["avatar_url"].toString();
         sAssignee.type = commentData["user"].toObject()["type"].toString();

         c.creator = std::move(sAssignee);
         comments.append(std::move(c));
      }

      issue.comments = comments;

      emit commentsReceived(issue);
   }
}

void GitHubRestApi::onReviewsReceived(PullRequest pr)
{
   const auto reply = qobject_cast<QNetworkReply *>(sender());
   const auto url = reply->url();
   QString errorStr;
   const auto tmpDoc = validateData(reply, errorStr);

   if (!tmpDoc.isEmpty())
   {
      QMap<int, Review> reviews;
      const auto commentsArray = tmpDoc.array();

      for (const auto &commentData : commentsArray)
      {
         auto id = commentData["id"].toInt();

         Review r;
         r.id = id;
         r.body = commentData["body"].toString();
         r.creation = commentData["created_at"].toVariant().toDateTime();

         GitServer::User sAssignee;
         sAssignee.id = commentData["user"].toObject()["id"].toInt();
         sAssignee.url = commentData["user"].toObject()["html_url"].toString();
         sAssignee.name = commentData["user"].toObject()["login"].toString();
         sAssignee.avatar = commentData["user"].toObject()["avatar_url"].toString();
         sAssignee.type = commentData["user"].toObject()["type"].toString();

         r.creator = std::move(sAssignee);
         reviews.insert(id, std::move(r));
      }

      pr.reviews = reviews;

      QTimer::singleShot(200, this, [this, pr]() { requestReviewComments(pr); });
   }
}

void GitHubRestApi::requestReviewComments(const PullRequest &pr)
{
   const auto reply = mManager->get(createRequest(mRepoEndpoint + QString("/pulls/%1/comments").arg(pr.number)));

   connect(reply, &QNetworkReply::finished, this, [this, pr]() { onReviewCommentsReceived(pr); });
}

void GitHubRestApi::onReviewCommentsReceived(PullRequest pr)
{
   const auto reply = qobject_cast<QNetworkReply *>(sender());
   const auto url = reply->url();
   QString errorStr;
   const auto tmpDoc = validateData(reply, errorStr);

   if (!tmpDoc.isEmpty())
   {
      QVector<ReviewComment> comments;
      const auto commentsArray = tmpDoc.array();

      for (const auto &commentData : commentsArray)
      {
         ReviewComment c;
         c.id = commentData["id"].toInt();
         c.body = commentData["body"].toString();
         c.creation = commentData["created_at"].toVariant().toDateTime();
         c.association = commentData["author_association"].toString();
         c.diff.diff = commentData["diff_hunk"].toString();
         c.diff.file = commentData["path"].toString();
         c.diff.position = commentData["position"].toInt();
         c.diff.originalPosition = commentData["original_position"].toInt();
         c.reviewId = commentData["pull_request_review_id"].toInt();
         c.replyToId = commentData["in_reply_to_id"].toInt();

         GitServer::User sAssignee;
         sAssignee.id = commentData["user"].toObject()["id"].toInt();
         sAssignee.url = commentData["user"].toObject()["html_url"].toString();
         sAssignee.name = commentData["user"].toObject()["login"].toString();
         sAssignee.avatar = commentData["user"].toObject()["avatar_url"].toString();
         sAssignee.type = commentData["user"].toObject()["type"].toString();

         c.creator = std::move(sAssignee);
         comments.append(std::move(c));
      }

      pr.reviewComment = comments;

      emit reviewsReceived(pr);
   }
}
