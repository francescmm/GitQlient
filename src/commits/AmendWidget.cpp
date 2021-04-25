#include <AmendWidget.h>
#include <ui_CommitChangesWidget.h>

#include <GitCache.h>
#include <GitLocal.h>
#include <GitQlientRole.h>
#include <GitWip.h>
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
   const auto commit = mCache->commitInfo(sha);

   ui->amendFrame->setVisible(true);
   ui->warningButton->setVisible(true);

   if (commit.parentsCount() <= 0)
      return;

   QScopedPointer<GitWip> git(new GitWip(mGit, mCache));
   git->updateWip();

   const auto files = mCache->revisionFile(CommitInfo::ZERO_SHA, sha);
   const auto amendFiles = mCache->revisionFile(sha, commit.firstParent());

   if (mCurrentSha != sha)
   {
      QLog_Info("UI", QString("Amending sha {%1}.").arg(mCurrentSha));

      mCurrentSha = sha;

      const auto author = commit.author.split("<");
      ui->leAuthorName->setText(author.first());
      ui->leAuthorEmail->setText(author.last().mid(0, author.last().count() - 1));
      ui->teDescription->setPlainText(commit.longLog.trimmed());
      ui->leCommitTitle->setText(commit.shortLog);

      blockSignals(true);
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
         QScopedPointer<GitWip> git(new GitWip(mGit, mCache));
         git->updateWip();

         const auto files = mCache->revisionFile(CommitInfo::ZERO_SHA, mCurrentSha);
         const auto author = QString("%1<%2>").arg(ui->leAuthorName->text(), ui->leAuthorEmail->text());

         QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

         QScopedPointer<GitLocal> gitLocal(new GitLocal(mGit));
         const auto ret = gitLocal->ammendCommit(selFiles, files, msg, author);
         QApplication::restoreOverrideCursor();

         emit signalChangesCommitted(ret.success);

         done = true;
      }
   }

   return done;
}
