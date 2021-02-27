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
   ui->applyActionBtn->setText(tr("Amend"));
}

void AmendWidget::configure(const QString &sha)
{
   const auto commit = mCache->getCommitInfo(sha);

   ui->amendFrame->setVisible(true);
   ui->warningButton->setVisible(true);

   if (commit.parentsCount() <= 0)
      return;

   if (!mCache->containsRevisionFile(CommitInfo::ZERO_SHA, sha))
   {
      QScopedPointer<GitLocal> gitLocal(new GitLocal(mGit));
      mCache->setUntrackedFilesList(gitLocal->getUntrackedFiles());

      if (const auto wipInfo = gitLocal->getWipDiff(); wipInfo.isValid())
         mCache->updateWipCommit(wipInfo);
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

   ui->applyActionBtn->setEnabled(ui->stagedFilesList->count());
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
         QScopedPointer<GitLocal> git(new GitLocal(mGit));
         mCache->setUntrackedFilesList(git->getUntrackedFiles());

         if (const auto wipInfo = git->getWipDiff(); wipInfo.isValid())
            mCache->updateWipCommit(wipInfo);

         const auto files = mCache->getRevisionFile(CommitInfo::ZERO_SHA, mCurrentSha);
         const auto author = QString("%1<%2>").arg(ui->leAuthorName->text(), ui->leAuthorEmail->text());

         QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
         const auto ret = git->ammendCommit(selFiles, files, msg, author);
         QApplication::restoreOverrideCursor();

         emit signalChangesCommitted(ret.success);

         done = true;
      }
   }

   return done;
}
