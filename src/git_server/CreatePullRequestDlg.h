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

#include <QDialog>
#include <ServerMilestone.h>
#include <ServerLabel.h>

namespace Ui
{
class CreatePullRequestDlg;
}

class GitBase;
class IRestApi;
class RevisionsCache;

/**
 * @brief The CreatePullRequestDlg class configures the UI so the user can create pull requests in the git remote
 * server.
 */
class CreatePullRequestDlg : public QDialog
{
   Q_OBJECT

signals:
   /**
    * @brief signalRefreshPRsCache Signal that refreshes pull requests cache.
    */
   void signalRefreshPRsCache();

public:
   /**
    * @brief CreatePullRequestDlg Default constructor,
    * @param cache The internal GitQlient cache,
    * @param git The git object to perform Git operations.
    * @param parent The parent widget.
    */
   explicit CreatePullRequestDlg(const QSharedPointer<RevisionsCache> &cache, const QSharedPointer<GitBase> &git,
                                 QWidget *parent = nullptr);
   /**
    * Destructor
    */
   ~CreatePullRequestDlg();

private:
   Ui::CreatePullRequestDlg *ui;
   QSharedPointer<RevisionsCache> mCache;
   QSharedPointer<GitBase> mGit;
   IRestApi *mApi;
   QString mUserName;
   int mIssue;
   QString mFinalUrl;

   /**
    * @brief accept Checks the data introduced by the user and connects to the server to create a pull request.
    */
   void accept() override;
   /**
    * @brief onMilestones Process the reply from the server when the milestones request is done.
    * @param milestones The list of milestones to process.
    */
   void onMilestones(const QVector<ServerMilestone> &milestones);
   /**
    * @brief onLabels Process the reply from the server when the labels request is done.
    * @param labels The list of labels to process.
    */
   void onLabels(const QVector<ServerLabel> &labels);
   /**
    * @brief onPullRequestCreated Shows a message box with the url of the pull request already created.
    * @param url The url that links to the pull request.
    */
   void onPullRequestCreated(QString url);
   /**
    * @brief onPullRequestUpdated On GitHub, some parameters in the Pull Request are configured in an second update
    * call. This indicates the widget that that update was successfully and triggers the message box to inform of the
    * link to the PR.
    */
   void onPullRequestUpdated();
   /**
    * @brief onGitServerError Notifies the user that an error happened in the API connection or data exchange.
    */
   void onGitServerError(const QString &error);
};
