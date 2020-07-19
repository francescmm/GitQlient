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
class CreateIssueDlg;
}

class GitBase;
class IRestApi;

/**
 * @brief The CreateIssueDlg class presents the UI where the user can create issues in the remote Git server.
 */
class CreateIssueDlg : public QDialog
{
   Q_OBJECT

public:
   /**
    * @brief CreateIssueDlg Default constructor.
    * @param git The git object to perform Git operations.
    * @param parent The parent widget.
    */
   explicit CreateIssueDlg(const QSharedPointer<GitBase> git, QWidget *parent = nullptr);
   /**
    * Destructor.
    */
   ~CreateIssueDlg();

private:
   Ui::CreateIssueDlg *ui;
   QSharedPointer<GitBase> mGit;
   IRestApi *mApi;
   QString mUserName;

   /**
    * @brief accept Process the user input data and does a request to create an issue.
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
    * @brief onIssueCreated Process the reply from the server if the issue was successfully created. It shows a message
    * box with the url of the issue.
    * @param url The url of the issue.
    */
   void onIssueCreated(QString url);

   /**
    * @brief onGitServerError Notifies the user that an error happened in the API connection or data exchange.
    */
   void onGitServerError(const QString &error);
};
