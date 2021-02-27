#include <WipWidget.h>
#include <ui_CommitChangesWidget.h>

#include <GitQlientRole.h>
#include <GitCache.h>
#include <GitRepoLoader.h>
#include <GitLocal.h>
#include <UnstagedMenu.h>
#include <GitBase.h>
#include <FileWidget.h>
#include <GitQlientStyles.h>

#include <QMessageBox>

#include <QLogger.h>

using namespace QLogger;

WipWidget::WipWidget(const QSharedPointer<GitCache> &cache, const QSharedPointer<GitBase> &git, QWidget *parent)
   : CommitChangesWidget(cache, git, parent)
{
   mCurrentSha = CommitInfo::ZERO_SHA;
}

void WipWidget::configure(const QString &sha)
{
   const auto commit = mCache->getCommitInfo(sha);

   if (!mCache->containsRevisionFile(CommitInfo::ZERO_SHA, commit.parent(0)))
   {
      QScopedPointer<GitLocal> gitLocal(new GitLocal(mGit));
      mCache->setUntrackedFilesList(gitLocal->getUntrackedFiles());

      if (const auto wipInfo = gitLocal->getWipDiff(); wipInfo.isValid())
         mCache->updateWipCommit(wipInfo);
   }

   const auto files = mCache->getRevisionFile(CommitInfo::ZERO_SHA, commit.parent(0));

   QLog_Info("UI", QString("Configuring WIP widget"));

   prepareCache();

   insertFiles(files, ui->unstagedFilesList);

   clearCache();

   ui->applyActionBtn->setEnabled(ui->stagedFilesList->count());
}

bool WipWidget::commitChanges()
{
   QString msg;
   QStringList selFiles = getFiles();
   auto done = false;

   if (!selFiles.isEmpty())
   {
      if (hasConflicts())
         QMessageBox::warning(this, tr("Impossible to commit"),
                              tr("There are files with conflicts. Please, resolve the conflicts first."));
      else if (checkMsg(msg))
      {
         const auto revInfo = mCache->getCommitInfo(CommitInfo::ZERO_SHA);

         QScopedPointer<GitLocal> gitLocal(new GitLocal(mGit));
         mCache->setUntrackedFilesList(gitLocal->getUntrackedFiles());

         if (const auto wipInfo = gitLocal->getWipDiff(); wipInfo.isValid())
            mCache->updateWipCommit(wipInfo);

         const auto files = mCache->getRevisionFile(CommitInfo::ZERO_SHA, revInfo.parent(0));

         QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
         QScopedPointer<GitLocal> git(new GitLocal(mGit));
         const auto ret = git->commitFiles(selFiles, files, msg);
         QApplication::restoreOverrideCursor();

         lastMsgBeforeError = (ret.success ? "" : msg);

         emit signalChangesCommitted(ret.success);

         done = true;

         ui->leCommitTitle->clear();
         ui->teDescription->clear();
      }
   }

   return done;
}
