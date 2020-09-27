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
      QScopedPointer<GitRepoLoader> git(new GitRepoLoader(mGit, mCache));
      git->updateWipRevision();
   }

   const auto files = mCache->getRevisionFile(CommitInfo::ZERO_SHA, commit.parent(0));

   QLog_Info("UI", QString("Updating files for SHA {%1}").arg(mCurrentSha));

   prepareCache();

   insertFiles(files, ui->unstagedFilesList);

   clearCache();

   ui->lUnstagedCount->setText(QString("(%1)").arg(ui->unstagedFilesList->count()));
   ui->lStagedCount->setText(QString("(%1)").arg(ui->stagedFilesList->count()));
   ui->pbCommit->setEnabled(ui->stagedFilesList->count());
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
         QScopedPointer<GitRepoLoader> gitLoader(new GitRepoLoader(mGit, mCache));
         gitLoader->updateWipRevision();
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

void WipWidget::showUnstagedMenu(const QPoint &pos)
{
   const auto item = ui->unstagedFilesList->itemAt(pos);

   if (item)
   {
      const auto fileName = item->toolTip();
      const auto unsolvedConflicts = item->data(GitQlientRole::U_IsConflict).toBool();
      const auto contextMenu = new UnstagedMenu(mGit, fileName, unsolvedConflicts, this);
      connect(contextMenu, &UnstagedMenu::signalEditFile, this,
              [this, fileName]() { emit signalEditFile(mGit->getWorkingDir() + "/" + fileName, 0, 0); });
      connect(contextMenu, &UnstagedMenu::signalShowDiff, this, &WipWidget::requestDiff);
      connect(contextMenu, &UnstagedMenu::signalCommitAll, this, &WipWidget::addAllFilesToCommitList);
      connect(contextMenu, &UnstagedMenu::signalRevertAll, this, &WipWidget::revertAllChanges);
      connect(contextMenu, &UnstagedMenu::signalCheckedOut, this, &WipWidget::signalCheckoutPerformed);
      connect(contextMenu, &UnstagedMenu::signalShowFileHistory, this, &WipWidget::signalShowFileHistory);
      connect(contextMenu, &UnstagedMenu::signalStageFile, this, [this, item] { addFileToCommitList(item); });
      connect(contextMenu, &UnstagedMenu::signalConflictsResolved, this, [this, item] {
         item->setData(GitQlientRole::U_IsConflict, false);
         item->setForeground(GitQlientStyles::getGreen());
         addFileToCommitList(item);
      });

      const auto parentPos = ui->unstagedFilesList->mapToParent(pos);
      contextMenu->popup(mapToGlobal(parentPos));
   }
}
