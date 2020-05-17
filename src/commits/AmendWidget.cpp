#include <AmendWidget.h>
#include <ui_WorkInProgressWidget.h>

#include <GitRepoLoader.h>
#include <GitBase.h>
#include <GitLocal.h>
#include <GitQlientStyles.h>
#include <CommitInfo.h>
#include <RevisionFiles.h>
#include <UnstagedMenu.h>
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

const int AmendWidget::kMaxTitleChars = 50;

QString AmendWidget::lastMsgBeforeError;

enum GitQlientRole
{
   U_ListRole = Qt::UserRole,
   U_IsConflict,
   U_Name
};

AmendWidget::AmendWidget(const QSharedPointer<RevisionsCache> &cache, const QSharedPointer<GitBase> &git,
                         QWidget *parent)
   : QWidget(parent)
   , ui(new Ui::WorkInProgressWidget)
   , mCache(cache)
   , mGit(git)
{
   ui->setupUi(this);
   setAttribute(Qt::WA_DeleteOnClose);

   ui->lCounter->setText(QString::number(kMaxTitleChars));
   ui->leCommitTitle->setMaxLength(kMaxTitleChars);
   ui->teDescription->setMaximumHeight(125);

   QIcon stagedIcon(":/icons/staged");
   ui->stagedFilesIcon->setPixmap(stagedIcon.pixmap(15, 15));

   QIcon unstagedIcon(":/icons/unstaged");
   ui->unstagedIcon->setPixmap(unstagedIcon.pixmap(15, 15));

   QIcon untrackedIcon(":/icons/untracked");
   ui->untrackedFilesIcon->setPixmap(untrackedIcon.pixmap(15, 15));

   connect(ui->leCommitTitle, &QLineEdit::textChanged, this, &AmendWidget::updateCounter);
   connect(ui->leCommitTitle, &QLineEdit::returnPressed, this, &AmendWidget::amendChanges);
   connect(ui->pbCommit, &QPushButton::clicked, this, &AmendWidget::amendChanges);
   connect(ui->untrackedFilesList, &QListWidget::itemDoubleClicked, this, &AmendWidget::onOpenDiffRequested);
   connect(ui->untrackedFilesList, &QListWidget::customContextMenuRequested, this, &AmendWidget::showUntrackedMenu);
   connect(ui->unstagedFilesList, &QListWidget::customContextMenuRequested, this, &AmendWidget::showUnstagedMenu);
   connect(ui->stagedFilesList, &QListWidget::customContextMenuRequested, this, &AmendWidget::showStagedMenu);
   connect(ui->unstagedFilesList, &QListWidget::itemDoubleClicked, this, &AmendWidget::onOpenDiffRequested);
   connect(ui->stagedFilesList, &QListWidget::itemDoubleClicked, this, &AmendWidget::onOpenDiffRequested);
   connect(ui->pbCancelAmend, &QPushButton::clicked, this, [this]() { emit signalCancelAmend(mCurrentSha); });

   ui->pbCommit->setText(tr("Amend"));
}

AmendWidget::~AmendWidget()
{
   delete ui;
}

void AmendWidget::reload()
{
   configure(mCurrentSha);
}

void AmendWidget::configure(const QString &sha)
{
   const auto commit = mCache->getCommitInfo(sha);

   if (commit.parentsCount() > 0)
   {
      mCurrentSha = sha;

      QLog_Info("UI", QString("Configuring commit widget with sha {%1} as amend").arg(mCurrentSha));

      blockSignals(true);
      mCurrentFilesCache.clear();
      ui->untrackedFilesList->clear();
      ui->unstagedFilesList->clear();
      ui->stagedFilesList->clear();
      blockSignals(false);

      // set-up files list
      const auto author = commit.author().split("<");
      ui->leAuthorName->setText(author.first());
      ui->leAuthorEmail->setText(author.last().mid(0, author.last().count() - 1));

      RevisionFiles files;
      const auto wipCommit = mCache->getCommitInfo(CommitInfo::ZERO_SHA);

      if (mCache->containsRevisionFile(CommitInfo::ZERO_SHA, wipCommit.parent(0)))
         files = mCache->getRevisionFile(CommitInfo::ZERO_SHA, wipCommit.parent(0));
      else if (commit.parentsCount() > 0)
      {
         QScopedPointer<GitRepoLoader> git(new GitRepoLoader(mGit, mCache));
         git->updateWipRevision();
         files = mCache->getRevisionFile(CommitInfo::ZERO_SHA, wipCommit.parent(0));
      }

      prepareCache();

      insertFilesInList(files, ui->unstagedFilesList);

      clearCache();

      const auto amendFiles = mCache->getRevisionFile(mCurrentSha, commit.parent(0));
      insertFilesInList(amendFiles, ui->stagedFilesList);

      ui->lUnstagedCount->setText(QString("(%1)").arg(ui->unstagedFilesList->count()));
      ui->lStagedCount->setText(QString("(%1)").arg(ui->stagedFilesList->count()));

      // compute cursor offsets. Take advantage of fixed width font
      QString msg = lastMsgBeforeError;

      if (lastMsgBeforeError.isEmpty())
      {
         QPair<QString, QString> logMessage;

         const auto revInfo = mCache->getCommitInfo(mCurrentSha);
         logMessage = qMakePair(revInfo.shortLog(), revInfo.longLog().trimmed());

         msg = logMessage.second.trimmed();

         ui->leCommitTitle->setText(logMessage.first);
      }

      ui->teDescription->setPlainText(msg);
      ui->teDescription->moveCursor(QTextCursor::Start);
      ui->pbCommit->setEnabled(ui->stagedFilesList->count());
   }
}

void AmendWidget::resetFile(QListWidgetItem *item)
{
   QScopedPointer<GitLocal> git(new GitLocal(mGit));
   const auto ret = git->resetFile(item->toolTip());
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
            fileWidget->setTextColor(item->foreground().color());

            connect(fileWidget, &FileWidget::clicked, this, [this, item]() { addFileToCommitList(item); });
            ui->unstagedFilesList->setItemWidget(item, fileWidget);
         }
         else if (untrackedFile)
         {
            item->setData(GitQlientRole::U_ListRole, QVariant::fromValue(ui->untrackedFilesList));

            ui->stagedFilesList->takeItem(row);
            ui->untrackedFilesList->addItem(item);

            const auto fileWidget = new FileWidget(iconPath, item->toolTip());
            fileWidget->setTextColor(item->foreground().color());

            connect(fileWidget, &FileWidget::clicked, this, [this, item]() { addFileToCommitList(item); });
            ui->untrackedFilesList->setItemWidget(item, fileWidget);
         }
      }
   }

   if (ret.success)
      emit signalUpdateWip();
}

QColor AmendWidget::getColorForFile(const RevisionFiles &files, int index) const
{
   const auto isUnknown = files.statusCmp(index, RevisionFiles::UNKNOWN);
   const auto isInIndex = files.statusCmp(index, RevisionFiles::IN_INDEX);
   const auto isConflict = files.statusCmp(index, RevisionFiles::CONFLICT);
   const auto untrackedFile = !isInIndex && isUnknown;

   QColor myColor;
   const auto isDeleted = files.statusCmp(index, RevisionFiles::DELETED);

   if (isConflict)
   {
      myColor = GitQlientStyles::getBlue();
   }
   else if (isDeleted)
      myColor = GitQlientStyles::getRed();
   else if (untrackedFile)
      myColor = GitQlientStyles::getOrange();
   else if (files.statusCmp(index, RevisionFiles::NEW) || isUnknown || isInIndex)
      myColor = GitQlientStyles::getGreen();
   else
      myColor = GitQlientStyles::getTextColor();

   return myColor;
}

void AmendWidget::insertFilesInList(const RevisionFiles &files, QListWidget *fileList)
{
   for (auto i = 0; i < files.count(); ++i)
   {
      const auto fileName = files.getFile(i);

      if (!mCurrentFilesCache.contains(fileName))
      {
         const auto isUnknown = files.statusCmp(i, RevisionFiles::UNKNOWN);
         const auto isInIndex = files.statusCmp(i, RevisionFiles::IN_INDEX);
         const auto isConflict = files.statusCmp(i, RevisionFiles::CONFLICT);
         const auto untrackedFile = !isInIndex && isUnknown;
         const auto staged = isInIndex && !isUnknown && !isConflict;

         QListWidgetItem *item = nullptr;

         if (untrackedFile)
         {
            item = new QListWidgetItem(ui->untrackedFilesList);
            item->setData(GitQlientRole::U_ListRole, QVariant::fromValue(ui->untrackedFilesList));
         }
         else if (staged)
         {
            item = new QListWidgetItem(ui->stagedFilesList);
            item->setData(GitQlientRole::U_ListRole, QVariant::fromValue(ui->stagedFilesList));
         }
         else
         {
            item = new QListWidgetItem(fileList);
            item->setData(GitQlientRole::U_ListRole, QVariant::fromValue(fileList));
         }

         if (isConflict)
            item->setData(GitQlientRole::U_IsConflict, true);

         item->setText(fileName);
         item->setData(GitQlientRole::U_Name, fileName);
         item->setToolTip(fileName);

         if (fileList == ui->stagedFilesList)
            item->setFlags(item->flags() & (~Qt::ItemIsSelectable & ~Qt::ItemIsEnabled));

         mCurrentFilesCache.insert(fileName, qMakePair(true, item));

         if (item->data(GitQlientRole::U_IsConflict).toBool())
         {
            const auto newName = item->text().append(" (conflicts)");
            item->setText(newName);
            item->setData(GitQlientRole::U_Name, newName);
         }

         const auto iconPath = !untrackedFile && staged ? QString(":/icons/remove") : QString(":/icons/add");
         const auto fileWidget = new FileWidget(iconPath, item->text());

         const auto textColor = getColorForFile(files, i);
         fileWidget->setTextColor(textColor);
         item->setForeground(textColor);

         if (staged)
            connect(fileWidget, &FileWidget::clicked, this, [this, item]() { resetFile(item); });
         else
            connect(fileWidget, &FileWidget::clicked, this, [this, item]() { addFileToCommitList(item); });

         qvariant_cast<QListWidget *>(item->data(GitQlientRole::U_ListRole))->setItemWidget(item, fileWidget);
         item->setText("");
         item->setSizeHint(fileWidget->sizeHint());
      }
      else
         mCurrentFilesCache[fileName].first = true;
   }
}

void AmendWidget::prepareCache()
{
   for (auto file = mCurrentFilesCache.begin(); file != mCurrentFilesCache.end(); ++file)
      file.value().first = false;
}

void AmendWidget::clearCache()
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

void AmendWidget::addAllFilesToCommitList()
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

void AmendWidget::onOpenDiffRequested(QListWidgetItem *item)
{
   requestDiff(item->toolTip());
}

void AmendWidget::requestDiff(const QString &fileName)
{
   emit signalShowDiff(CommitInfo::ZERO_SHA, mCache->getCommitInfo(CommitInfo::ZERO_SHA).parent(0), fileName);
}

void AmendWidget::addFileToCommitList(QListWidgetItem *item)
{
   const auto fileList = qvariant_cast<QListWidget *>(item->data(GitQlientRole::U_ListRole));
   const auto row = fileList->row(item);
   const auto fileWidget = qobject_cast<FileWidget *>(fileList->itemWidget(item));
   const auto newFileWidget = new FileWidget(":/icons/remove", fileWidget->text());
   newFileWidget->setTextColor(item->foreground().color());

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

void AmendWidget::revertAllChanges()
{
   auto i = ui->unstagedFilesList->count() - 1;

   for (; i >= 0; --i)
   {
      const auto fileName = ui->unstagedFilesList->takeItem(i)->toolTip();
      QScopedPointer<GitLocal> git(new GitLocal(mGit));
      const auto ret = git->checkoutFile(fileName);

      emit signalCheckoutPerformed(ret);
   }
}

void AmendWidget::removeFileFromCommitList(QListWidgetItem *item)
{
   if (item->flags() & Qt::ItemIsSelectable)
   {
      const auto itemOriginalList = qvariant_cast<QListWidget *>(item->data(GitQlientRole::U_ListRole));
      const auto row = ui->stagedFilesList->row(item);
      const auto fileWidget = qobject_cast<FileWidget *>(ui->stagedFilesList->itemWidget(item));
      const auto newFileWidget = new FileWidget(":/icons/add", fileWidget->text());
      newFileWidget->setTextColor(item->foreground().color());

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

void AmendWidget::showUntrackedMenu(const QPoint &pos)
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

void AmendWidget::showStagedMenu(const QPoint &pos)
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
            /*
            connect(menu->addAction("Unstage file"), &QAction::triggered, this,
                    [this, item] { removeFileFromCommitList(item); });
            */

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

QStringList AmendWidget::getFiles()
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

bool AmendWidget::checkMsg(QString &msg)
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

void AmendWidget::updateCounter(const QString &text)
{
   ui->lCounter->setText(QString::number(kMaxTitleChars - text.count()));
}

bool AmendWidget::hasConflicts()
{
   const auto files = mCurrentFilesCache.values();

   for (const auto &pair : files)
   {
      if (pair.second->data(GitQlientRole::U_IsConflict).toBool())
         return true;
   }

   return false;
}

bool AmendWidget::amendChanges()
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

void AmendWidget::clear()
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
