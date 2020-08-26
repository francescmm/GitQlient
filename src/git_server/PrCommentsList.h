#pragma once

/****************************************************************************************
 ** GitQlient is an application to manage and operate one or several Git repositories. With
 ** GitQlient you will be able to add commits, branches and manage all the options Git provides.
 ** Copyright (C) 2020  Francesc Martinez
 **
 ** LinkedIn: www.linkedin.com/in/cescmm/
 ** Web: www.francescmm.com
 **
 ** This program is free software; you can redistribute it and/or
 ** modify it under the terms of the GNU Lesser General Public
 ** License as published by the Free Software Foundation; either
 ** version 2 of the License, or (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 ** Lesser General Public License for more details.
 **
 ** You should have received a copy of the GNU Lesser General Public
 ** License along with this library; if not, write to the Free Software
 ** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 ***************************************************************************************/

#include <Issue.h>
#include <GitServerCache.h>

#include <QFrame>

namespace GitServer
{
struct Issue;
struct PullRequest;
struct Comment;
struct Review;
struct CodeReview;
}

class QLabel;
class QVBoxLayout;
class QNetworkAccessManager;
class QHBoxLayout;
class QScrollArea;

class PrCommentsList : public QFrame
{
public:
   enum class Config
   {
      Issues,
      PullRequests
   };

   explicit PrCommentsList(const QSharedPointer<GitServerCache> &gitServerCache, QWidget *parent = nullptr);

   void loadData(Config config, int issueNumber);

private:
   QSharedPointer<GitServerCache> mGitServerCache;
   QNetworkAccessManager *mManager = nullptr;
   QVBoxLayout *mIssuesLayout = nullptr;
   QFrame *mIssuesFrame = nullptr;
   Config mConfig;
   QScrollArea *mScroll = nullptr;
   bool mLoaded = false;
   int mIssueNumber = -1;

   void processComments(const GitServer::Issue &issue);
   QLabel *createHeadline(const QDateTime &dt, const QString &prefix = QString());
   void onReviewsReceived(GitServer::PullRequest pr);
   QLayout *createBubbleForComment(const GitServer::Comment &comment);
   QLayout *createBubbleForReview(const GitServer::Review &review);
   QVector<QLayout *> createBubbleForCodeReview(int reviewId, QVector<GitServer::CodeReview> &comments);
};
