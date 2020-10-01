#include <AmendWidget.h>
#include <ui_CommitChangesWidget.h>

#include <GitCache.h>
#include <GitRepoLoader.h>
#include <GitBase.h>
#include <GitLocal.h>
#include <GitQlientRole.h>
#include <UnstagedMenu.h>

#include <QMessageBox>

#include <QLogger.h>

using namespace QLogger;

AmendWidget::AmendWidget(const QSharedPointer<GitCache> &cache, const QSharedPointer<GitBase> &git, QWidget *parent)
   : CommitChangesWidget(cache, git, parent)
{
   ui->pbCommit->setText(tr("Amend"));
}

void AmendWidget::configure(const QString &sha)
{
   const auto commit = mCache->getCommitInfo(sha);

   ui->amendFrame->setVisible(true);
   ui->pbCancelAmend->setVisible(true);

   if (commit.parentsCount() <= 0)
      return;

   if (!mCache->containsRevisionFile(CommitInfo::ZERO_SHA, sha))
   {
      QScopedPointer<GitRepoLoader> git(new GitRepoLoader(mGit, mCache));
      git->updateWipRevision();
   }

   const auto files = mCache->getRevisionFile(CommitInfo::ZERO_SHA, sha);
   const auto amendFiles = mCache->getRevisionFile(sha, commit.parent(0));

   if (mCurrentSha != sha)
   {
      QLog_Info("UI", QString("Amending sha {%1}.").arg(mCurrentSha));

      mCurrentSha = sha;

      const auto author = commit.author().split("<");
      ui->leAuthorName->setText(author.first());
      ui->leAuthorEmail->setText(author.last().mid(0, author.last().count() - 1));
      ui->teDescription->setPlainText(commit.longLog().trimmed());
      ui->leCommitTitle->setText(commit.shortLog());

      blockSignals(true);
      mInternalCache.clear();
      ui->untrackedFilesList->clear();
      ui->unstagedFilesList->clear();
      ui->stagedFilesList->clear();
      blockSignals(false);

      insertFiles(files, ui->unstagedFilesList);
      insertFiles(amendFiles, ui->stagedFilesList);
   }
   else
   {
      QLog_Info("UI", QString("Updating files for SHA {%1}").arg(mCurrentSha));

      prepareCache();

      insertFiles(files, ui->unstagedFilesList);

      clearCache();

      insertFiles(amendFiles, ui->stagedFilesList);
   }

   ui->lUnstagedCount->setText(QString("(%1)").arg(ui->unstagedFilesList->count()));
   ui->lStagedCount->setText(QString("(%1)").arg(ui->stagedFilesList->count()));

   ui->pbCommit->setEnabled(ui->stagedFilesList->count());
}

bool AmendWidget::commitChanges()
{
   QStringList selFiles = getFiles();
   auto done = false;

   if (!selFiles.isEmpty())
   {
      QString msg;

      if (hasConflicts())
      {
         QMessageBox::critical(this, tr("Impossible to commit"),
                               tr("There are files with conflicts. Please, resolve the conflicts first."));
      }
      else if (checkMsg(msg))
      {
         QScopedPointer<GitRepoLoader> gitLoader(new GitRepoLoader(mGit, mCache));
         gitLoader->updateWipRevision();
         const auto files = mCache->getRevisionFile(CommitInfo::ZERO_SHA, mCurrentSha);
         const auto author = QString("%1<%2>").arg(ui->leAuthorName->text(), ui->leAuthorEmail->text());

         QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
         QScopedPointer<GitLocal> git(new GitLocal(mGit));
         const auto ret = git->ammendCommit(selFiles, files, msg, author);
         QApplication::restoreOverrideCursor();

         emit signalChangesCommitted(ret.success);

         done = true;
      }
   }

   return done;
}

void AmendWidget::showUnstagedMenu(const QPoint &pos)
{
   const auto item = ui->unstagedFilesList->itemAt(pos);

   if (item)
   {
      const auto fileName = item->toolTip();
      const auto unsolvedConflicts = item->data(GitQlientRole::U_IsConflict).toBool();
      const auto contextMenu = new UnstagedMenu(mGit, fileName, unsolvedConflicts, this);
      connect(contextMenu, &UnstagedMenu::signalEditFile, this,
              [this, fileName]() { emit signalEditFile(mGit->getWorkingDir() + "/" + fileName, 0, 0); });
      connect(contextMenu, &UnstagedMenu::signalShowDiff, this, &AmendWidget::requestDiff);
      connect(contextMenu, &UnstagedMenu::signalCommitAll, this, &AmendWidget::addAllFilesToCommitList);
      connect(contextMenu, &UnstagedMenu::signalRevertAll, this, &AmendWidget::revertAllChanges);
      connect(contextMenu, &UnstagedMenu::signalCheckedOut, this, &AmendWidget::signalCheckoutPerformed);
      connect(contextMenu, &UnstagedMenu::signalShowFileHistory, this, &AmendWidget::signalShowFileHistory);
      connect(contextMenu, &UnstagedMenu::signalStageFile, this, [this, item] { addFileToCommitList(item); });

      const auto parentPos = ui->unstagedFilesList->mapToParent(pos);
      contextMenu->popup(mapToGlobal(parentPos));
   }
}
