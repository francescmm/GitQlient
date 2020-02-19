#include <WorkInProgressWidget.h>
#include <ui_WorkInProgressWidget.h>

#include <GitRepoLoader.h>
#include <GitBase.h>
#include <GitLocal.h>
#include <GitQlientStyles.h>
#include <CommitInfo.h>
#include <RevisionFiles.h>
#include <UnstagedMenu.h>
#include <FileListDelegate.h>
#include <RevisionsCache.h>
#include <FileWidget.h>

#include <QDir>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QRegExp>
#include <QScrollBar>
#include <QTextCodec>
#include <QToolTip>
#include <QListWidgetItem>
#include <QTextStream>
#include <QProcess>
#include <QItemDelegate>
#include <QPainter>

#include <QLogger.h>

using namespace QLogger;

const int WorkInProgressWidget::kMaxTitleChars = 50;

QString WorkInProgressWidget::lastMsgBeforeError;

enum GitQlientRole
{
   U_ListRole = Qt::UserRole,
   U_IsConflict,
   U_Name
};

WorkInProgressWidget::WorkInProgressWidget(const QSharedPointer<RevisionsCache> &cache,
                                           const QSharedPointer<GitBase> &git, QWidget *parent)
   : QWidget(parent)
   , ui(new Ui::WorkInProgressWidget)
   , mCache(cache)
   , mGit(git)
{
   ui->setupUi(this);

   ui->unstagedFilesList->setItemDelegate(new FileListDelegate(this));
   ui->stagedFilesList->setItemDelegate(new FileListDelegate(this));
   ui->untrackedFilesList->setItemDelegate(new FileListDelegate(this));

   ui->lCounter->setText(QString::number(kMaxTitleChars));
   ui->leCommitTitle->setMaxLength(kMaxTitleChars);
   ui->teDescription->setMaximumHeight(125);

   QIcon stagedIcon(":/icons/staged");
   ui->stagedFilesIcon->setPixmap(stagedIcon.pixmap(15, 15));

   QIcon unstagedIcon(":/icons/unstaged");
   ui->unstagedIcon->setPixmap(unstagedIcon.pixmap(15, 15));

   QIcon untrackedIcon(":/icons/untracked");
   ui->untrackedFilesIcon->setPixmap(untrackedIcon.pixmap(15, 15));

   connect(ui->leCommitTitle, &QLineEdit::textChanged, this, &WorkInProgressWidget::updateCounter);
   connect(ui->leCommitTitle, &QLineEdit::returnPressed, this, &WorkInProgressWidget::applyChanges);
   connect(ui->pbCommit, &QPushButton::clicked, this, &WorkInProgressWidget::applyChanges);
   connect(ui->untrackedFilesList, &QListWidget::itemDoubleClicked, this, &WorkInProgressWidget::onOpenDiffRequested);
   connect(ui->untrackedFilesList, &QListWidget::customContextMenuRequested, this,
           &WorkInProgressWidget::showUntrackedMenu);
   connect(ui->unstagedFilesList, &QListWidget::customContextMenuRequested, this,
           &WorkInProgressWidget::showUnstagedMenu);
   connect(ui->stagedFilesList, &QListWidget::customContextMenuRequested, this, &WorkInProgressWidget::showStagedMenu);
   connect(ui->unstagedFilesList, &QListWidget::itemDoubleClicked, this, &WorkInProgressWidget::onOpenDiffRequested);
   connect(ui->stagedFilesList, &QListWidget::itemDoubleClicked, this, &WorkInProgressWidget::onOpenDiffRequested);
}

void WorkInProgressWidget::configure(const QString &sha)
{
   if (mCache->getCommitInfo(sha).parentsCount() > 0)
   {
      const auto shaChange = mCurrentSha != sha;
      mCurrentSha = sha;

      resetInfo(shaChange);
   }
}

void WorkInProgressWidget::resetInfo(bool force)
{
   mIsAmend = mCurrentSha != CommitInfo::ZERO_SHA;

   QLog_Info("UI",
             QString("Configuring commit widget with sha {%1} as {%2}")
                 .arg(mCurrentSha, mIsAmend ? QString("amend") : QString("wip")));

   blockSignals(true);

   if (mIsAmend)
   {
      mCurrentFilesCache.clear();
      ui->untrackedFilesList->clear();
      ui->unstagedFilesList->clear();
      ui->stagedFilesList->clear();
   }

   ui->leAuthorName->setVisible(mIsAmend);
   ui->leAuthorEmail->setVisible(mIsAmend);
   blockSignals(false);

   ui->pbCommit->setText(mIsAmend ? QString("Amend") : QString("Commit"));

   // set-up files list
   const auto revInfo = mCache->getCommitInfo(mCurrentSha);

   if (mIsAmend)
   {
      const auto author = revInfo.author().split("<");
      ui->leAuthorName->setText(author.first());
      ui->leAuthorEmail->setText(author.last().mid(0, author.last().count() - 1));
   }

   RevisionFiles files;

   if (mCache->containsRevisionFile(CommitInfo::ZERO_SHA, revInfo.parent(0)))
      files = mCache->getRevisionFile(CommitInfo::ZERO_SHA, revInfo.parent(0));
   else if (revInfo.parentsCount() > 0)
   {
      QScopedPointer<GitRepoLoader> git(new GitRepoLoader(mGit, mCache));
      git->updateWipRevision();
      files = mCache->getRevisionFile(CommitInfo::ZERO_SHA, revInfo.parent(0));
   }

   if (!force || (mIsAmend && force))
      prepareCache();

   insertFilesInList(files, ui->unstagedFilesList);

   if (!force || (mIsAmend && force))
      clearCache();

   if (mIsAmend)
   {
      const auto amendFiles = mCache->getRevisionFile(mCurrentSha, revInfo.parent(0));
      insertFilesInList(amendFiles, ui->stagedFilesList);
   }

   ui->lUnstagedCount->setText(QString("(%1)").arg(ui->unstagedFilesList->count()));
   ui->lStagedCount->setText(QString("(%1)").arg(ui->stagedFilesList->count()));

   // compute cursor offsets. Take advantage of fixed width font
   QString msg;

   if (lastMsgBeforeError.isEmpty())
   {
      QPair<QString, QString> logMessage;

      if (mIsAmend)
      {
         const auto revInfo = mCache->getCommitInfo(mCurrentSha);
         logMessage = qMakePair(revInfo.shortLog(), revInfo.longLog().trimmed());
      }

      msg = logMessage.second.trimmed();

      if (mIsAmend || force)
         ui->leCommitTitle->setText(logMessage.first);
   }
   else
      msg = lastMsgBeforeError;

   if (mIsAmend || force)
   {
      ui->teDescription->setPlainText(msg);
      ui->teDescription->moveCursor(QTextCursor::Start);
   }

   ui->pbCommit->setEnabled(ui->stagedFilesList->count());
}

void WorkInProgressWidget::resetFile(QListWidgetItem *item)
{
   QScopedPointer<GitLocal> git(new GitLocal(mGit));
   const auto ret = git->resetFile(item->toolTip());

   if (ret.success)
      emit signalCheckoutPerformed(ret.success);

   const auto revInfo = mCache->getCommitInfo(mCurrentSha);
   const auto files = mCache->getRevisionFile(mCurrentSha, revInfo.parent(0));

   for (auto i = 0; i < files.count(); ++i)
   {
      auto fileName = files.getFile(i);

      if (fileName == item->toolTip())
      {
         const auto isUnknown = files.statusCmp(i, RevisionFiles::UNKNOWN);
         const auto isInIndex = files.statusCmp(i, RevisionFiles::IN_INDEX);
         const auto untrackedFile = !isInIndex && isUnknown;
         const auto row = ui->stagedFilesList->row(item);
         const auto iconPath = QString(":/icons/add");

         if (isInIndex)
         {
            item->setData(GitQlientRole::U_ListRole, QVariant::fromValue(ui->unstagedFilesList));

            ui->stagedFilesList->takeItem(row);
            ui->unstagedFilesList->addItem(item);

            const auto fileWidget = new FileWidget(iconPath, item->toolTip());
            connect(fileWidget, &FileWidget::clicked, this, [this, item]() { addFileToCommitList(item); });
            ui->unstagedFilesList->setItemWidget(item, fileWidget);
         }
         else if (untrackedFile)
         {
            item->setData(GitQlientRole::U_ListRole, QVariant::fromValue(ui->untrackedFilesList));

            ui->stagedFilesList->takeItem(row);
            ui->untrackedFilesList->addItem(item);

            const auto fileWidget = new FileWidget(iconPath, item->toolTip());
            connect(fileWidget, &FileWidget::clicked, this, [this, item]() { addFileToCommitList(item); });

            ui->untrackedFilesList->setItemWidget(item, fileWidget);
         }
      }
   }
}

void WorkInProgressWidget::insertFilesInList(const RevisionFiles &files, QListWidget *fileList)
{
   for (auto i = 0; i < files.count(); ++i)
   {
      auto fileName = files.getFile(i);

      if (!mCurrentFilesCache.contains(fileName))
      {
         const auto isUnknown = files.statusCmp(i, RevisionFiles::UNKNOWN);
         const auto isInIndex = files.statusCmp(i, RevisionFiles::IN_INDEX);
         const auto isConflict = files.statusCmp(i, RevisionFiles::CONFLICT);
         const auto untrackedFile = !isInIndex && isUnknown;
         const auto staged = isInIndex && !isUnknown && !isConflict;

         auto iconPath = QString(":/icons/add");
         QListWidgetItem *item = nullptr;

         if (untrackedFile)
         {
            item = new QListWidgetItem(ui->untrackedFilesList);
            item->setData(GitQlientRole::U_ListRole, QVariant::fromValue(ui->untrackedFilesList));
         }
         else if (staged)
         {
            iconPath = QString(":/icons/remove");
            item = new QListWidgetItem(ui->stagedFilesList);
            item->setData(GitQlientRole::U_ListRole, QVariant::fromValue(ui->stagedFilesList));
         }
         else
         {
            item = new QListWidgetItem(fileList);
            item->setData(GitQlientRole::U_ListRole, QVariant::fromValue(fileList));
         }

         QColor myColor;
         const auto isDeleted = files.statusCmp(i, RevisionFiles::DELETED);

         if ((files.statusCmp(i, RevisionFiles::NEW) || isUnknown || isInIndex) && !untrackedFile && !isDeleted
             && !isConflict)
            myColor = GitQlientStyles::getGreen();
         else if (isConflict)
         {
            myColor = GitQlientStyles::getBlue();
            item->setData(GitQlientRole::U_IsConflict, true);
         }
         else if (isDeleted)
            myColor = GitQlientStyles::getRed();
         else if (untrackedFile)
            myColor = GitQlientStyles::getOrange();
         else
            myColor = GitQlientStyles::getTextColor();

         item->setText(fileName);
         item->setData(GitQlientRole::U_Name, fileName);
         item->setToolTip(fileName);
         item->setForeground(myColor);

         if (mIsAmend && fileList == ui->stagedFilesList)
            item->setFlags(item->flags() & (~Qt::ItemIsSelectable & ~Qt::ItemIsEnabled));

         mCurrentFilesCache.insert(fileName, qMakePair(true, item));

         if (item->data(GitQlientRole::U_IsConflict).toBool())
         {
            const auto newName = item->text().append(" (conflicts)");
            item->setText(newName);
            item->setData(GitQlientRole::U_Name, newName);
         }

         const auto fileWidget = new FileWidget(iconPath, item->text());

         if (staged)
            connect(fileWidget, &FileWidget::clicked, this, [this, item]() { resetFile(item); });
         else
            connect(fileWidget, &FileWidget::clicked, this, [this, item]() { addFileToCommitList(item); });

         qvariant_cast<QListWidget *>(item->data(GitQlientRole::U_ListRole))->setItemWidget(item, fileWidget);
         item->setText("");
      }
      else
         mCurrentFilesCache[fileName].first = true;
   }
}

void WorkInProgressWidget::prepareCache()
{
   for (auto file = mCurrentFilesCache.begin(); file != mCurrentFilesCache.end(); ++file)
      file.value().first = false;
}

void WorkInProgressWidget::clearCache()
{
   for (auto it = mCurrentFilesCache.begin(); it != mCurrentFilesCache.end();)
   {
      if (!it.value().first)
      {
         delete it.value().second;
         it = mCurrentFilesCache.erase(it);
      }
      else
         ++it;
   }
}

void WorkInProgressWidget::addAllFilesToCommitList()
{
   auto i = ui->unstagedFilesList->count() - 1;

   for (; i >= 0; --i)
   {
      const auto item = ui->unstagedFilesList->item(i);

      addFileToCommitList(item);
   }

   ui->lUnstagedCount->setText(QString("(%1)").arg(ui->unstagedFilesList->count()));
   ui->lStagedCount->setText(QString("(%1)").arg(ui->stagedFilesList->count()));
   ui->pbCommit->setEnabled(ui->stagedFilesList->count() > 0);
}

void WorkInProgressWidget::onOpenDiffRequested(QListWidgetItem *item)
{
   requestDiff(item->toolTip());
}

void WorkInProgressWidget::requestDiff(const QString &fileName)
{
   emit signalShowDiff(CommitInfo::ZERO_SHA, mCache->getCommitInfo(CommitInfo::ZERO_SHA).parent(0), fileName);
}

void WorkInProgressWidget::addFileToCommitList(QListWidgetItem *item)
{
   const auto fileList = qvariant_cast<QListWidget *>(item->data(GitQlientRole::U_ListRole));
   const auto row = fileList->row(item);
   const auto fileWidget = qobject_cast<FileWidget *>(fileList->itemWidget(item));
   const auto newFileWidget = new FileWidget(":/icons/remove", fileWidget->text());
   connect(newFileWidget, &FileWidget::clicked, this, [this, item]() { removeFileFromCommitList(item); });

   fileList->removeItemWidget(item);
   fileList->takeItem(row);

   ui->stagedFilesList->addItem(item);
   ui->stagedFilesList->setItemWidget(item, newFileWidget);

   if (item->data(GitQlientRole::U_IsConflict).toBool())
      newFileWidget->setText(newFileWidget->text().remove("(conflicts)").trimmed());

   delete fileWidget;

   ui->lUntrackedCount->setText(QString("(%1)").arg(ui->untrackedFilesList->count()));
   ui->lUnstagedCount->setText(QString("(%1)").arg(ui->unstagedFilesList->count()));
   ui->lStagedCount->setText(QString("(%1)").arg(ui->stagedFilesList->count()));
   ui->pbCommit->setEnabled(true);
}

void WorkInProgressWidget::revertAllChanges()
{
   auto i = ui->unstagedFilesList->count() - 1;

   for (; i >= 0; --i)
   {
      const auto fileName = ui->unstagedFilesList->takeItem(i)->data(Qt::DisplayRole).toString();
      QScopedPointer<GitLocal> git(new GitLocal(mGit));
      const auto ret = git->checkoutFile(fileName);

      emit signalCheckoutPerformed(ret);
   }
}

void WorkInProgressWidget::removeFileFromCommitList(QListWidgetItem *item)
{
   if (item->flags() & Qt::ItemIsSelectable)
   {
      const auto itemOriginalList = qvariant_cast<QListWidget *>(item->data(GitQlientRole::U_ListRole));
      const auto row = ui->stagedFilesList->row(item);
      const auto fileWidget = qobject_cast<FileWidget *>(ui->stagedFilesList->itemWidget(item));
      const auto newFileWidget = new FileWidget(":/icons/add", fileWidget->text());
      connect(newFileWidget, &FileWidget::clicked, this, [this, item]() { addFileToCommitList(item); });

      if (item->data(GitQlientRole::U_IsConflict).toBool())
         newFileWidget->setText(fileWidget->text().append(" (conflicts)"));

      delete fileWidget;

      ui->stagedFilesList->removeItemWidget(item);
      const auto item = ui->stagedFilesList->takeItem(row);

      itemOriginalList->addItem(item);
      itemOriginalList->setItemWidget(item, newFileWidget);

      ui->lUnstagedCount->setText(QString("(%1)").arg(ui->unstagedFilesList->count()));
      ui->lStagedCount->setText(QString("(%1)").arg(ui->stagedFilesList->count()));
      ui->pbCommit->setDisabled(ui->stagedFilesList->count() == 0);
   }
}

void WorkInProgressWidget::showUnstagedMenu(const QPoint &pos)
{
   const auto item = ui->unstagedFilesList->itemAt(pos);

   if (item)
   {
      const auto fileName = item->toolTip();
      const auto unsolvedConflicts = item->data(GitQlientRole::U_IsConflict).toBool();
      const auto contextMenu = new UnstagedMenu(mGit, fileName, unsolvedConflicts, this);
      connect(contextMenu, &UnstagedMenu::signalShowDiff, this, &WorkInProgressWidget::requestDiff);
      connect(contextMenu, &UnstagedMenu::signalCommitAll, this, &WorkInProgressWidget::addAllFilesToCommitList);
      connect(contextMenu, &UnstagedMenu::signalRevertAll, this, &WorkInProgressWidget::revertAllChanges);
      connect(contextMenu, &UnstagedMenu::signalCheckedOut, this, &WorkInProgressWidget::signalCheckoutPerformed);
      connect(contextMenu, &UnstagedMenu::signalShowFileHistory, this, &WorkInProgressWidget::signalShowFileHistory);
      connect(contextMenu, &UnstagedMenu::signalStageFile, this, [this, item] { addFileToCommitList(item); });
      connect(contextMenu, &UnstagedMenu::signalConflictsResolved, this, [this, item] {
         const auto fileWidget = qobject_cast<FileWidget *>(ui->unstagedFilesList->itemWidget(item));

         item->setData(GitQlientRole::U_IsConflict, false);
         item->setText(fileWidget->text().remove("(conflicts)").trimmed());
         item->setForeground(GitQlientStyles::getGreen());
         resetInfo();
      });

      const auto parentPos = ui->unstagedFilesList->mapToParent(pos);
      contextMenu->popup(mapToGlobal(parentPos));
   }
}

void WorkInProgressWidget::showUntrackedMenu(const QPoint &pos)
{
   const auto item = ui->untrackedFilesList->itemAt(pos);

   if (item)
   {
      const auto fileName = item->toolTip();
      const auto contextMenu = new QMenu(this);
      connect(contextMenu->addAction(tr("Stage file")), &QAction::triggered, this,
              [this, item]() { addFileToCommitList(item); });
      connect(contextMenu->addAction(tr("Delete file")), &QAction::triggered, this, [this, fileName]() {
         QProcess p;
         p.setWorkingDirectory(mGit->getWorkingDir());
         p.start(QString("rm -rf %1").arg(fileName));
         if (p.waitForFinished())
            emit this->signalCheckoutPerformed(true);
      });

      const auto parentPos = ui->untrackedFilesList->mapToParent(pos);
      contextMenu->popup(mapToGlobal(parentPos));
   }
}

void WorkInProgressWidget::showStagedMenu(const QPoint &pos)
{
   const auto item = ui->stagedFilesList->itemAt(pos);

   if (item)
   {
      const auto fileName = item->toolTip();
      const auto menu = new QMenu(this);

      if (item->flags() & Qt::ItemIsSelectable)
      {
         const auto itemOriginalList = qvariant_cast<QListWidget *>(item->data(GitQlientRole::U_ListRole));

         if (sender() == itemOriginalList)
         {
            const auto resetAction = menu->addAction("Reset");
            connect(resetAction, &QAction::triggered, this, [this, item] { resetFile(item); });
         }
         else
         {
            connect(menu->addAction("Unstage file"), &QAction::triggered, this,
                    [this, item] { removeFileFromCommitList(item); });

            if (itemOriginalList != ui->untrackedFilesList)
            {
               connect(menu->addAction("See changes"), &QAction::triggered, this, [this, fileName]() {
                  emit signalShowDiff(CommitInfo::ZERO_SHA, mCache->getCommitInfo(CommitInfo::ZERO_SHA).parent(0),
                                      fileName);
               });
            }
         }
      }

      const auto parentPos = ui->stagedFilesList->mapToParent(pos);
      menu->popup(mapToGlobal(parentPos));
   }
}

void WorkInProgressWidget::applyChanges()
{
   mIsAmend ? amendChanges() : commitChanges();
}

QStringList WorkInProgressWidget::getFiles()
{
   QStringList selFiles;
   const auto totalItems = ui->stagedFilesList->count();

   for (auto i = 0; i < totalItems; ++i)
   {
      const auto fileWidget = static_cast<FileWidget *>(ui->stagedFilesList->itemWidget(ui->stagedFilesList->item(i)));
      selFiles.append(fileWidget->text());
   }

   return selFiles;
}

bool WorkInProgressWidget::checkMsg(QString &msg)
{
   const auto title = ui->leCommitTitle->text();

   if (title.isEmpty())
      QMessageBox::warning(this, "Commit changes", "Please, add a title.");

   msg = title;

   if (!ui->teDescription->toPlainText().isEmpty())
   {
      auto description = QString("\n\n%1").arg(ui->teDescription->toPlainText());
      description.remove(QRegExp("(^|\\n)\\s*#[^\\n]*")); // strip comments
      msg += description;
   }

   msg.replace(QRegExp("[ \\t\\r\\f\\v]+\\n"), "\n"); // strip line trailing cruft
   msg = msg.trimmed();

   if (msg.isEmpty())
   {
      QMessageBox::warning(this, "Commit changes", "Please, add a title.");
      return false;
   }

   QString subj(msg.section('\n', 0, 0, QString::SectionIncludeTrailingSep));
   QString body(msg.section('\n', 1).trimmed());
   msg = subj + '\n' + body + '\n';

   return true;
}

void WorkInProgressWidget::updateCounter(const QString &text)
{
   ui->lCounter->setText(QString::number(kMaxTitleChars - text.count()));
}

bool WorkInProgressWidget::hasConflicts()
{
   const auto files = mCurrentFilesCache.values();

   for (const auto &pair : files)
   {
      if (pair.second->data(GitQlientRole::U_IsConflict).toBool())
         return true;
   }

   return false;
}

bool WorkInProgressWidget::commitChanges()
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
         const auto ret = git->commitFiles(selFiles, files, msg, false);
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

bool WorkInProgressWidget::amendChanges()
{
   QStringList selFiles = getFiles();
   auto done = false;

   if (!selFiles.isEmpty())
   {
      QString msg;

      if (hasConflicts())
         QMessageBox::warning(this, tr("Impossible to commit"),
                              tr("There are files with conflicts. Please, resolve the conflicts first."));
      else if (checkMsg(msg))
      {
         const auto author = QString("%1<%2>").arg(ui->leAuthorName->text(), ui->leAuthorEmail->text());

         const auto revInfo = mCache->getCommitInfo(CommitInfo::ZERO_SHA);
         QScopedPointer<GitRepoLoader> gitLoader(new GitRepoLoader(mGit, mCache));
         gitLoader->updateWipRevision();
         const auto files = mCache->getRevisionFile(CommitInfo::ZERO_SHA, revInfo.parent(0));

         QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
         QScopedPointer<GitLocal> git(new GitLocal(mGit));
         const auto ret = git->commitFiles(selFiles, files, msg, true, author);
         QApplication::restoreOverrideCursor();

         emit signalChangesCommitted(ret.success);

         done = true;
      }
   }

   return done;
}

void WorkInProgressWidget::clear()
{
   ui->untrackedFilesList->clear();
   ui->unstagedFilesList->clear();
   ui->stagedFilesList->clear();
   mCurrentFilesCache.clear();
   ui->leCommitTitle->clear();
   ui->leAuthorName->clear();
   ui->leAuthorEmail->clear();
   ui->teDescription->clear();
   ui->pbCommit->setEnabled(false);
   ui->lStagedCount->setText(QString("(%1)").arg(ui->unstagedFilesList->count()));
   ui->lUnstagedCount->setText(QString("(%1)").arg(ui->stagedFilesList->count()));
   ui->lUntrackedCount->setText(QString("(%1)").arg(ui->unstagedFilesList->count()));
}
