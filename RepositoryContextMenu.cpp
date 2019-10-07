#include "RepositoryContextMenu.h"

#include <git.h>
#include <common.h>
#include <CommitWidget.h>
#include <BranchDlg.h>
#include <TagDlg.h>

#include <QMessageBox>
#include <QApplication>
#include <QClipboard>

RepositoryContextMenu::RepositoryContextMenu(const QString &sha, QWidget *parent)
   : QMenu(parent)
{
   if (sha == QGit::ZERO_SHA)
   {
      const auto stashAction = new QAction("Push stash");
      connect(stashAction, &QAction::triggered, this, [this]() {
         const auto ret = Git::getInstance()->stash();

         if (ret)
            emit signalRepositoryUpdated();
      });
      addAction(stashAction);

      const auto popAction = new QAction("Pop stash");
      connect(popAction, &QAction::triggered, this, [this]() {
         const auto ret = Git::getInstance()->pop();

         if (ret)
            emit signalRepositoryUpdated();
      });
      addAction(popAction);

      const auto commitAction = new QAction("Commit");
      // connect(commitAction, &QAction::triggered, this, [this]() {});
      addAction(commitAction);
   }

   const auto commitAction = new QAction("See diff");
   connect(commitAction, &QAction::triggered, this, [this, sha]() { emit signalOpenDiff(sha); });
   addAction(commitAction);

   if (sha != QGit::ZERO_SHA)
   {
      const auto ammendCommitAction = new QAction("Ammend");
      connect(ammendCommitAction, &QAction::triggered, this, &RepositoryContextMenu::signalAmmendCommit);
      addAction(ammendCommitAction);

      const auto createBranchAction = new QAction("Create branch here");
      connect(createBranchAction, &QAction::triggered, this, [this, sha]() {
         BranchDlg dlg(sha, BranchDlgMode::CREATE_FROM_COMMIT);
         const auto ret = dlg.exec();

         if (ret == QDialog::Accepted)
            emit signalRepositoryUpdated();
      });
      addAction(createBranchAction);

      const auto createTagAction = new QAction("Create tag here");
      connect(createTagAction, &QAction::triggered, this, [this, sha]() {
         TagDlg dlg(sha);
         const auto ret = dlg.exec();

         if (ret == QDialog::Accepted)
            emit signalRepositoryUpdated();
      });
      addAction(createTagAction);
      addSeparator();

      QByteArray output;
      const auto ret = Git::getInstance()->getBranchesOfCommit(sha, output);
      const auto currentBranch = Git::getInstance()->getCurrentBranchName();

      if (ret)
      {
         auto branches = QString::fromUtf8(output).split('\n');

         for (auto &branch : branches)
         {
            if (branch.contains("*"))
               branch.remove("*");

            if (branch.contains("->"))
            {
               branch.clear();
               continue;
            }

            branch.remove("remotes/");
            branch = branch.trimmed();

            if (!branch.isEmpty() && branch != currentBranch && branch != QString("origin/%1").arg(currentBranch))
            {
               const auto checkoutCommitAction = new QAction(QString("Checkout %1").arg(branch));
               checkoutCommitAction->setDisabled(true);
               // connect(checkoutCommitAction, &QAction::triggered, this, &RepositoryView::executeAction);
               addAction(checkoutCommitAction);
            }
         }

         for (auto branch : qAsConst(branches))
         {
            if (!branch.isEmpty() && branch != currentBranch && branch != QString("origin/%1").arg(currentBranch))
            {
               // If is the last commit of a branch
               const auto mergeBranchAction = new QAction(QString("Merge %1").arg(branch));
               mergeBranchAction->setDisabled(true);
               // connect(mergeBranchAction, &QAction::triggered, this, &RepositoryView::executeAction);
               addAction(mergeBranchAction);
            }
         }

         addSeparator();

         auto isCommitInCurrentBranch = false;

         for (auto branch : qAsConst(branches))
            isCommitInCurrentBranch |= branch == currentBranch;

         if (!isCommitInCurrentBranch)
         {
            const auto cherryPickAction = new QAction("Cherry pick commit");
            connect(cherryPickAction, &QAction::triggered, this, [this, sha]() {
               if (Git::getInstance()->cherryPickCommit(sha))
                  emit signalRepositoryUpdated();
            });
            addAction(cherryPickAction);
         }
      }

      QByteArray lastSha;
      if (Git::getInstance()->getLastCommitOfBranch(currentBranch, lastSha))
      {
         const auto lastShaStr = QString::fromUtf8(lastSha).remove('\n');

         if (lastShaStr == sha)
         {
            const auto pushAction = new QAction("Push");
            connect(pushAction, &QAction::triggered, this, [this]() {
               QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
               const auto ret = Git::getInstance()->push();
               QApplication::restoreOverrideCursor();

               if (ret)
                  emit signalRepositoryUpdated();
            });
            addAction(pushAction);

            const auto pullAction = new QAction("Pull");
            connect(pullAction, &QAction::triggered, this, [this]() {
               QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
               QString output;
               const auto ret = Git::getInstance()->pull(output);
               QApplication::restoreOverrideCursor();

               if (ret)
                  emit signalRepositoryUpdated();
            });
            addAction(pullAction);

            const auto fetchAction = new QAction("Fetch");
            connect(fetchAction, &QAction::triggered, this, [this]() {
               if (Git::getInstance()->fetch())
                  emit signalRepositoryUpdated();
            });
            addAction(fetchAction);
         }
      }

      const auto copyShaAction = new QAction("Copy SHA");
      connect(copyShaAction, &QAction::triggered, this, [sha]() { QApplication::clipboard()->setText(sha); });
      addAction(copyShaAction);
      addSeparator();

      const auto resetSoftAction = new QAction("Reset - Soft");
      connect(resetSoftAction, &QAction::triggered, this, [this, sha]() {
         if (Git::getInstance()->resetCommit(sha, Git::CommitResetType::SOFT))
            emit signalRepositoryUpdated();
      });
      addAction(resetSoftAction);

      const auto resetMixedAction = new QAction("Reset - Mixed");
      connect(resetMixedAction, &QAction::triggered, this, [this, sha]() {
         if (Git::getInstance()->resetCommit(sha, Git::CommitResetType::MIXED))
            emit signalRepositoryUpdated();
      });
      addAction(resetMixedAction);

      const auto resetHardAction = new QAction("Reset - Hard");
      connect(resetHardAction, &QAction::triggered, this, [this, sha]() {
         const auto retMsg
             = QMessageBox::warning(this, "Reset hard requested!",
                                    "Are you sure you want to reset the branch to this commit in a <b>hard</b> way?",
                                    QMessageBox::Ok, QMessageBox::Cancel);
         if (retMsg == QMessageBox::Ok)
         {
            if (Git::getInstance()->resetCommit(sha, Git::CommitResetType::HARD))
               emit signalRepositoryUpdated();
         }
      });
      addAction(resetHardAction);
   }
}
