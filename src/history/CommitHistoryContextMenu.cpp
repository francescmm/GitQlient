#include "CommitHistoryContextMenu.h"

#include <GitServerCache.h>
#include <GitQlientStyles.h>
#include <GitLocal.h>
#include <GitTags.h>
#include <GitPatches.h>
#include <GitBase.h>
#include <GitStashes.h>
#include <GitBranches.h>
#include <GitRemote.h>
#include <BranchDlg.h>
#include <TagDlg.h>
#include <CommitInfo.h>
#include <GitCache.h>
#include <PullDlg.h>
#include <CreateIssueDlg.h>
#include <CreatePullRequestDlg.h>
#include <GitHubRestApi.h>
#include <GitQlientSettings.h>
#include <MergePullRequestDlg.h>

#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QProcess>
#include <QDesktopServices>

#include <QLogger.h>

using namespace QLogger;

CommitHistoryContextMenu::CommitHistoryContextMenu(const QSharedPointer<GitCache> &cache,
                                                   const QSharedPointer<GitBase> &git,
                                                   const QSharedPointer<GitServerCache> &gitServerCache,
                                                   const QStringList &shas, QWidget *parent)
   : QMenu(parent)
   , mCache(cache)
   , mGit(git)
   , mGitServerCache(gitServerCache)
   , mShas(shas)
{
   setAttribute(Qt::WA_DeleteOnClose);

   if (shas.count() == 1)
      createIndividualShaMenu();
   else
      createMultipleShasMenu();
}

void CommitHistoryContextMenu::createIndividualShaMenu()
{
   const auto singleSelection = mShas.count() == 1;

   if (singleSelection)
   {
      const auto sha = mShas.first();

      if (sha == CommitInfo::ZERO_SHA)
      {
         const auto stashAction = addAction("Push stash");
         connect(stashAction, &QAction::triggered, this, &CommitHistoryContextMenu::stashPush);

         const auto popAction = addAction("Pop stash");
         connect(popAction, &QAction::triggered, this, &CommitHistoryContextMenu::stashPop);
      }

      const auto commitAction = addAction("See diff");
      connect(commitAction, &QAction::triggered, this, [this]() { emit signalOpenDiff(mShas.first()); });

      if (sha != CommitInfo::ZERO_SHA)
      {
         const auto createBranchAction = addAction("Create branch here");
         connect(createBranchAction, &QAction::triggered, this, &CommitHistoryContextMenu::createBranch);

         const auto createTagAction = addAction("Create tag here");
         connect(createTagAction, &QAction::triggered, this, &CommitHistoryContextMenu::createTag);

         const auto exportAsPatchAction = addAction("Export as patch");
         connect(exportAsPatchAction, &QAction::triggered, this, &CommitHistoryContextMenu::exportAsPatch);

         addSeparator();

         const auto checkoutCommitAction = addAction("Checkout commit");
         connect(checkoutCommitAction, &QAction::triggered, this, &CommitHistoryContextMenu::checkoutCommit);

         addBranchActions(sha);

         QScopedPointer<GitBranches> git(new GitBranches(mGit));
         const auto ret = git->getLastCommitOfBranch(mGit->getCurrentBranch());

         if (ret.success)
         {
            const auto lastShaStr = ret.output.toString().remove('\n');

            if (lastShaStr == sha)
            {
               const auto amendCommitAction = addAction("Amend");
               connect(amendCommitAction, &QAction::triggered, this,
                       [this]() { emit signalAmendCommit(mShas.first()); });

               const auto applyPatchAction = addAction("Apply patch");
               connect(applyPatchAction, &QAction::triggered, this, &CommitHistoryContextMenu::applyPatch);

               const auto applyCommitAction = addAction("Apply commit");
               connect(applyCommitAction, &QAction::triggered, this, &CommitHistoryContextMenu::applyCommit);

               const auto pushAction = addAction("Push");
               connect(pushAction, &QAction::triggered, this, &CommitHistoryContextMenu::push);

               const auto pullAction = addAction("Pull");
               connect(pullAction, &QAction::triggered, this, &CommitHistoryContextMenu::pull);

               const auto fetchAction = addAction("Fetch");
               connect(fetchAction, &QAction::triggered, this, &CommitHistoryContextMenu::fetch);
            }
         }

         const auto copyShaAction = addAction("Copy SHA");
         connect(copyShaAction, &QAction::triggered, this,
                 [this]() { QApplication::clipboard()->setText(mShas.first()); });

         const auto copyTitleAction = addAction("Copy commit title");
         connect(copyTitleAction, &QAction::triggered, this, [this]() {
            const auto title = mCache->getCommitInfo(mShas.first()).shortLog();
            QApplication::clipboard()->setText(title);
         });

         addSeparator();

         const auto resetSoftAction = addAction("Reset - Soft");
         connect(resetSoftAction, &QAction::triggered, this, &CommitHistoryContextMenu::resetSoft);

         const auto resetMixedAction = addAction("Reset - Mixed");
         connect(resetMixedAction, &QAction::triggered, this, &CommitHistoryContextMenu::resetMixed);

         const auto resetHardAction = addAction("Reset - Hard");
         connect(resetHardAction, &QAction::triggered, this, &CommitHistoryContextMenu::resetHard);
      }
   }

   if (mGitServerCache)
   {
      const auto isGitHub = mGitServerCache->getPlatform() == GitServer::Platform::GitHub;
      addSeparator();

      const auto gitServerMenu = new QMenu(QString::fromUtf8(isGitHub ? "GitHub" : "GitLab"), this);
      addMenu(gitServerMenu);

      if (const auto pr = mGitServerCache->getPullRequest(mShas.first()); singleSelection && pr.isValid())
      {
         const auto prInfo = mGitServerCache->getPullRequest(mShas.first());

         const auto checksMenu = new QMenu("Checks", gitServerMenu);
         gitServerMenu->addMenu(checksMenu);

         for (const auto &check : prInfo.state.checks)
         {
            const auto link = check.url;
            checksMenu->addAction(QIcon(QString(":/icons/%1").arg(check.state)), check.name, this,
                                  [link]() { QDesktopServices::openUrl(link); });
         }

         if (isGitHub)
         {
            connect(gitServerMenu->addAction("Merge PR"), &QAction::triggered, this, [this, pr]() {
               const auto mergeDlg = new MergePullRequestDlg(mGit, pr, mShas.first(), this);
               connect(mergeDlg, &MergePullRequestDlg::signalRepositoryUpdated, this,
                       &CommitHistoryContextMenu::signalRepositoryUpdated);

               mergeDlg->exec();
            });
         }

         const auto link = mGitServerCache->getPullRequest(mShas.first()).url;
         connect(gitServerMenu->addAction("Show PR detailed view"), &QAction::triggered, this,
                 [this, num = pr.number]() { emit showPrDetailedView(num); });

         gitServerMenu->addSeparator();
      }

      connect(gitServerMenu->addAction("New Issue"), &QAction::triggered, this, [this]() {
         const auto createIssue = new CreateIssueDlg(mGitServerCache, this);
         createIssue->exec();
      });
      connect(gitServerMenu->addAction("New Pull Request"), &QAction::triggered, this, [this]() {
         const auto prDlg
             = new CreatePullRequestDlg(mCache, mGitServerCache, mGit->getWorkingDir(), mGit->getCurrentBranch(), this);
         connect(prDlg, &CreatePullRequestDlg::signalRefreshPRsCache, this,
                 &CommitHistoryContextMenu::signalRefreshPRsCache);

         prDlg->exec();
      });
   }
}

void CommitHistoryContextMenu::createMultipleShasMenu()
{
   if (mShas.count() == 2)
   {
      const auto diffAction = addAction("See diff");
      connect(diffAction, &QAction::triggered, this, [this]() { emit signalOpenCompareDiff(mShas); });
   }

   if (!mShas.contains(CommitInfo::ZERO_SHA))
   {
      const auto exportAsPatchAction = addAction("Export as patch");
      connect(exportAsPatchAction, &QAction::triggered, this, &CommitHistoryContextMenu::exportAsPatch);

      const auto copyShaAction = addAction("Copy all SHA");
      connect(copyShaAction, &QAction::triggered, this,
              [this]() { QApplication::clipboard()->setText(mShas.join(',')); });
   }
   else
      QLog_Warning("UI", "WIP selected as part of a series of SHAs");
}

void CommitHistoryContextMenu::stashPush()
{
   QScopedPointer<GitStashes> git(new GitStashes(mGit));
   const auto ret = git->stash();

   if (ret.success)
      emit signalRepositoryUpdated();
}

void CommitHistoryContextMenu::stashPop()
{
   QScopedPointer<GitStashes> git(new GitStashes(mGit));
   const auto ret = git->pop();

   if (ret.success)
      emit signalRepositoryUpdated();
}

void CommitHistoryContextMenu::createBranch()
{
   BranchDlg dlg({ mShas.first(), BranchDlgMode::CREATE_FROM_COMMIT, mGit });
   const auto ret = dlg.exec();

   if (ret == QDialog::Accepted)
      emit signalRepositoryUpdated();
}

void CommitHistoryContextMenu::createTag()
{
   TagDlg dlg(QSharedPointer<GitBase>::create(mGit->getWorkingDir()), mShas.first());
   const auto ret = dlg.exec();

   if (ret == QDialog::Accepted)
      emit signalRepositoryUpdated();
}

void CommitHistoryContextMenu::exportAsPatch()
{
   QScopedPointer<GitPatches> git(new GitPatches(mGit));
   const auto ret = git->exportPatch(mShas);

   if (ret.success)
   {
      const auto action = QMessageBox::information(
          this, tr("Patch generated"),
          tr("<p>The patch has been generated!</p>"
             "<p><b>Commit:</b></p><p>%1</p>"
             "<p><b>Destination:</b> %2</p>"
             "<p><b>File names:</b></p><p>%3</p>")
              .arg(mShas.join("<br>"), mGit->getWorkingDir(), ret.output.toStringList().join("<br>")),
          QMessageBox::Ok, QMessageBox::Open);

      if (action == QMessageBox::Open)
      {
         QString fileBrowser;

#ifdef Q_OS_LINUX
         fileBrowser.append("xdg-open");
#elif defined(Q_OS_WIN)
         fileBrowser.append("explorer.exe");
#endif

         QProcess::startDetached(fileBrowser, { mGit->getWorkingDir() });
      }
   }
}

void CommitHistoryContextMenu::checkoutBranch()
{
   const auto action = qobject_cast<QAction *>(sender());
   auto isLocal = action->data().toBool();
   auto branchName = action->text();

   if (isLocal)
      branchName.remove("origin/");

   QScopedPointer<GitBranches> git(new GitBranches(mGit));
   const auto ret = isLocal ? git->checkoutLocalBranch(branchName) : git->checkoutRemoteBranch(branchName);
   const auto output = ret.output.toString();

   if (ret.success)
   {
      QRegExp rx("by \\d+ commits");
      rx.indexIn(ret.output.toString());
      auto value = rx.capturedTexts().constFirst().split(" ");

      if (value.count() == 3 && output.contains("your branch is behind", Qt::CaseInsensitive))
      {
         const auto commits = value.at(1).toUInt();
         (void)commits;

         PullDlg pull(mGit, output.split('\n').first());

         connect(&pull, &PullDlg::signalRepositoryUpdated, this, &CommitHistoryContextMenu::signalRepositoryUpdated);
         connect(&pull, &PullDlg::signalPullConflict, this, &CommitHistoryContextMenu::signalPullConflict);
      }

      emit signalRepositoryUpdated();
   }
   else
   {
      QMessageBox msgBox(QMessageBox::Critical, tr("Error while checking out"),
                         QString("There were problems during the checkout operation. Please, see the detailed "
                                 "description for more information."),
                         QMessageBox::Ok, this);
      msgBox.setDetailedText(ret.output.toString());
      msgBox.setStyleSheet(GitQlientStyles::getStyles());
      msgBox.exec();
   }
}

void CommitHistoryContextMenu::checkoutCommit()
{
   const auto sha = mShas.first();
   QLog_Info("UI", QString("Checking out the commit {%1}").arg(sha));

   QScopedPointer<GitLocal> git(new GitLocal(mGit));
   const auto ret = git->checkoutCommit(sha);

   if (ret.success)
      emit signalRepositoryUpdated();
   else
   {
      QMessageBox msgBox(QMessageBox::Critical, tr("Error while checking out"),
                         QString("There were problems during the checkout operation. Please, see the detailed "
                                 "description for more information."),
                         QMessageBox::Ok, this);
      msgBox.setDetailedText(ret.output.toString());
      msgBox.setStyleSheet(GitQlientStyles::getStyles());
      msgBox.exec();
   }
}

void CommitHistoryContextMenu::cherryPickCommit()
{
   QScopedPointer<GitLocal> git(new GitLocal(mGit));
   const auto ret = git->cherryPickCommit(mShas.first());

   if (ret.success)
      emit signalRepositoryUpdated();
   else
   {
      const auto errorMsg = ret.output.toString();

      if (errorMsg.contains("error: could not apply", Qt::CaseInsensitive)
          && errorMsg.contains("after resolving the conflicts", Qt::CaseInsensitive))
      {
         emit signalCherryPickConflict();
      }
      else
      {
         QMessageBox msgBox(QMessageBox::Critical, tr("Error while cherry-pick"),
                            QString("There were problems during the cherry-pich operation. Please, see the detailed "
                                    "description for more information."),
                            QMessageBox::Ok, this);
         msgBox.setDetailedText(errorMsg);
         msgBox.setStyleSheet(GitQlientStyles::getStyles());
         msgBox.exec();
      }
   }
}

void CommitHistoryContextMenu::applyPatch()
{
   const QString fileName(QFileDialog::getOpenFileName(this, "Select a patch to apply"));
   QScopedPointer<GitPatches> git(new GitPatches(mGit));

   if (!fileName.isEmpty() && git->applyPatch(fileName))
      emit signalRepositoryUpdated();
}

void CommitHistoryContextMenu::applyCommit()
{
   const QString fileName(QFileDialog::getOpenFileName(this, "Select a patch to apply"));
   QScopedPointer<GitPatches> git(new GitPatches(mGit));

   if (!fileName.isEmpty() && git->applyPatch(fileName, true))
      emit signalRepositoryUpdated();
}

void CommitHistoryContextMenu::push()
{
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   QScopedPointer<GitRemote> git(new GitRemote(mGit));
   const auto ret = git->push();
   QApplication::restoreOverrideCursor();

   if (ret.output.toString().contains("has no upstream branch"))
   {
      const auto currentBranch = mGit->getCurrentBranch();
      BranchDlg dlg({ currentBranch, BranchDlgMode::PUSH_UPSTREAM, mGit });
      const auto ret = dlg.exec();

      if (ret == QDialog::Accepted)
      {
         emit signalRefreshPRsCache();
         emit signalRepositoryUpdated();
      }
   }
   else if (ret.success)
   {
      emit signalRefreshPRsCache();
      emit signalRepositoryUpdated();
   }
   else
   {
      QMessageBox msgBox(QMessageBox::Critical, tr("Error while pushing"),
                         QString("There were problems during the push operation. Please, see the detailed description "
                                 "for more information."),
                         QMessageBox::Ok, this);
      msgBox.setDetailedText(ret.output.toString());
      msgBox.setStyleSheet(GitQlientStyles::getStyles());
      msgBox.exec();
   }
}

void CommitHistoryContextMenu::pull()
{
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   QScopedPointer<GitRemote> git(new GitRemote(mGit));
   const auto ret = git->pull();
   QApplication::restoreOverrideCursor();

   if (ret.success)
      emit signalRepositoryUpdated();
   else
   {
      const auto errorMsg = ret.output.toString();

      if (errorMsg.contains("error: could not apply", Qt::CaseInsensitive)
          && errorMsg.contains("causing a conflict", Qt::CaseInsensitive))
      {
         emit signalPullConflict();
      }
      else
      {
         QMessageBox msgBox(QMessageBox::Critical, tr("Error while pulling"),
                            QString("There were problems during the pull operation. Please, see the detailed "
                                    "description for more information."),
                            QMessageBox::Ok, this);
         msgBox.setDetailedText(errorMsg);
         msgBox.setStyleSheet(GitQlientStyles::getStyles());
         msgBox.exec();
      }
   }
}

void CommitHistoryContextMenu::fetch()
{
   QScopedPointer<GitRemote> git(new GitRemote(mGit));

   if (git->fetch())
   {
      QScopedPointer<GitTags> gitTags(new GitTags(mGit));
      const auto remoteTags = gitTags->getRemoteTags();

      mCache->updateTags(remoteTags);
      emit signalRepositoryUpdated();
   }
}

void CommitHistoryContextMenu::resetSoft()
{
   QScopedPointer<GitLocal> git(new GitLocal(mGit));

   if (git->resetCommit(mShas.first(), GitLocal::CommitResetType::SOFT))
      emit signalRepositoryUpdated();
}

void CommitHistoryContextMenu::resetMixed()
{
   QScopedPointer<GitLocal> git(new GitLocal(mGit));

   if (git->resetCommit(mShas.first(), GitLocal::CommitResetType::MIXED))
      emit signalRepositoryUpdated();
}

void CommitHistoryContextMenu::resetHard()
{
   const auto retMsg = QMessageBox::warning(
       this, "Reset hard requested!", "Are you sure you want to reset the branch to this commit in a <b>hard</b> way?",
       QMessageBox::Ok, QMessageBox::Cancel);

   if (retMsg == QMessageBox::Ok)
   {
      QScopedPointer<GitLocal> git(new GitLocal(mGit));

      if (git->resetCommit(mShas.first(), GitLocal::CommitResetType::HARD))
         emit signalRepositoryUpdated();
   }
}

void CommitHistoryContextMenu::merge(const QString &branchFrom)
{
   QScopedPointer<GitRemote> git(new GitRemote(mGit));
   const auto currentBranch = mGit->getCurrentBranch();

   emit signalMergeRequired(currentBranch, branchFrom);
}

void CommitHistoryContextMenu::addBranchActions(const QString &sha)
{
   const auto commitInfo = mCache->getCommitInfo(sha);
   auto remoteBranches = commitInfo.getReferences(References::Type::RemoteBranches);
   const auto localBranches = commitInfo.getReferences(References::Type::LocalBranch);

   QMap<QString, bool> branchTracking;

   if (remoteBranches.isEmpty())
   {
      for (const auto &branch : localBranches)
         branchTracking.insert(branch, true);
   }
   else
   {
      for (const auto &local : localBranches)
      {
         const auto iter = std::find_if(remoteBranches.begin(), remoteBranches.end(), [local](const QString &remote) {
            if (remote.contains(local))
               return true;
            return false;
         });

         branchTracking.insert(local, true);

         if (iter != remoteBranches.end())
            remoteBranches.erase(iter);
      }
      for (const auto &remote : remoteBranches)
         branchTracking.insert(remote, false);
   }

   QList<QAction *> branchesToCheckout;
   auto isCommitInCurrentBranch = false;
   const auto currentBranch = mGit->getCurrentBranch();

   for (const auto &pair : branchTracking.toStdMap())
   {
      isCommitInCurrentBranch |= pair.first == currentBranch;

      if (!branchTracking.isEmpty() && pair.first != currentBranch
          && pair.first != QString("origin/%1").arg(currentBranch))
      {
         const auto checkoutCommitAction = new QAction(QString(tr("%1")).arg(pair.first));
         checkoutCommitAction->setData(pair.second);
         connect(checkoutCommitAction, &QAction::triggered, this, &CommitHistoryContextMenu::checkoutBranch);
         branchesToCheckout.append(checkoutCommitAction);
      }
   }

   if (!branchesToCheckout.isEmpty())
   {
      const auto branchMenu = addMenu("Checkout branch...");
      branchMenu->addActions(branchesToCheckout);
   }

   if (!isCommitInCurrentBranch)
   {
      for (const auto &pair : branchTracking.toStdMap())
      {
         if (!pair.first.isEmpty() && pair.first != currentBranch
             && pair.first != QString("origin/%1").arg(currentBranch))
         {
            // If is the last commit of a branch
            const auto mergeBranchAction = addAction(QString(tr("Merge %1")).arg(pair.first));
            connect(mergeBranchAction, &QAction::triggered, this, [this, pair]() { merge(pair.first); });
         }
      }
   }

   addSeparator();

   if (!isCommitInCurrentBranch)
   {
      const auto cherryPickAction = addAction(tr("Cherry pick commit"));
      connect(cherryPickAction, &QAction::triggered, this, &CommitHistoryContextMenu::cherryPickCommit);
   }
}
