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

#include <QMenu>

class GitBase;

/*!
 \brief The BranchContextMenuConfig contains the necessary information to initialize the BranchContextMenu. It includes
 information about the current branch, the selected branch in the view and the git object to perform the Git actions.

*/
struct BranchContextMenuConfig
{
   QString currentBranch;
   QString branchSelected;
   bool isLocal;
   QSharedPointer<GitBase> mGit;
};

/*!
 \brief The BranchContextMenuConfig creates the context menu for the BranchTreeWidget. In this context menu all the
 possible actions regarding branches and it's workload are performed. This includes pushing pending commits to remote
 branches.

*/
class BranchContextMenu : public QMenu
{
   Q_OBJECT

signals:
   /*!
    \brief Signal triggered when the branches have been updated and the GitQlient UI needs a refresh.

   */
   void signalBranchesUpdated();
   /*!
    \brief Signal triggered when a branch has been checked out.

   */
   void signalCheckoutBranch();
   /*!
    \brief Signal triggered when the user wants to perform a merge. This action takes a \p fromBranch to merge it into
    our \ref currentBranch. In case of conflict, it will be handle elsewhere.

    \param currentBranch The current working branch.
    \param fromBranch The branch to be merge into the current branch.
   */
   void signalMergeRequired(const QString &currentBranch, const QString &fromBranch);
   /*!
    * \brief signalPullConflict Signal triggered when trying to pull and a conflict happens.
    */
   void signalPullConflict();

   /**
    * @brief signalFetchPerformed Signal triggered when a deep fetch is performed.
    */
   void signalFetchPerformed();
   /**
    * @brief signalRefreshPRsCache Signal that refreshes PRs cache.
    */
   void signalRefreshPRsCache();

public:
   /*!
    \brief Default constructor.

    \param config The data to configure the context menu.
    \param parent The parent widget if needed.
   */
   explicit BranchContextMenu(BranchContextMenuConfig config, QWidget *parent = nullptr);

private:
   BranchContextMenuConfig mConfig;

   /*!
    \brief Pulls the current branch.

   */
   void pull();
   /*!
    \brief Fetches all the changes from the remote repo. This includes gathering all tags as well, pruning and forcing
    the pruning.

   */
   void fetch();
   /*!
    \brief Pushes all the local changes to the remote repo.

   */
   void push();
   /*!
    \brief Pushes force all the local changes into the remote repo.

   */
   void pushForce();
   /*!
    \brief Creates a branch locally.

   */
   void createBranch();
   /*!
    \brief Creates a new local branch and checks it out.

   */
   void createCheckoutBranch();
   /*!
    \brief Tries to merge the selected branch in the BranchTreeWidget into the current branch.

   */
   void merge();
   /*!
    \brief Renames the selected branch.

   */
   void rename();
   /*!
    \brief Deletes the selected branch. It will fail if the branch to remove is the current one.

   */
   void deleteBranch();
};
