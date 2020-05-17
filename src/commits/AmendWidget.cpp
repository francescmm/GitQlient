#include <AmendWidget.h>
#include <ui_WorkInProgressWidget.h>

#include <GitRepoLoader.h>
#include <GitBase.h>
#include <GitLocal.h>
#include <GitHistory.h>
#include <GitQlientStyles.h>
#include <CommitInfo.h>
#include <RevisionFiles.h>
#include <UnstagedMenu.h>
#include <RevisionsCache.h>
#include <FileWidget.h>
#include <GitQlientRole.h>

#include <QMessageBox>
#include <QRegExp>
#include <QListWidgetItem>

#include <QLogger.h>

using namespace QLogger;

const int AmendWidget::kMaxTitleChars = 50;

QString AmendWidget::lastMsgBeforeError;

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

   ui->untrackedFilesList->setWorkingDirectory(mGit->getWorkingDir());

   connect(ui->leCommitTitle, &QLineEdit::textChanged, this, &AmendWidget::updateCounter);
   connect(ui->leCommitTitle, &QLineEdit::returnPressed, this, &AmendWidget::amendChanges);
   connect(ui->pbCommit, &QPushButton::clicked, this, &AmendWidget::amendChanges);
   connect(ui->untrackedFilesList, &UntrackedFilesList::signalShowDiff, this, &AmendWidget::requestDiff);
   connect(ui->untrackedFilesList, &UntrackedFilesList::signalStageFile, this, &AmendWidget::addFileToCommitList);
   connect(ui->untrackedFilesList, &UntrackedFilesList::signalCheckoutPerformed, this,
           &AmendWidget::signalCheckoutPerformed);
   connect(ui->stagedFilesList, &StagedFilesList::signalResetFile, this, &AmendWidget::resetFile);
   connect(ui->stagedFilesList, &StagedFilesList::signalShowDiff, this, &AmendWidget::requestDiff);
   connect(ui->unstagedFilesList, &QListWidget::customContextMenuRequested, this, &AmendWidget::showUnstagedMenu);
   connect(ui->unstagedFilesList, &QListWidget::itemDoubleClicked, this,
           [this](QListWidgetItem *item) { requestDiff(item->toolTip()); });
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

      blockSignals(true);
      mCurrentFilesCache.clear();
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

   if (lastMsgBeforeError.isEmpty())
   {
      ui->teDescription->setPlainText(commit.longLog().trimmed());
      ui->leCommitTitle->setText(commit.shortLog());
   }

   ui->teDescription->moveCursor(QTextCursor::Start);
   ui->pbCommit->setEnabled(ui->stagedFilesList->count());
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

void AmendWidget::insertFiles(const RevisionFiles &files, QListWidget *fileList)
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

         auto parent = fileList;

         if (untrackedFile)
            parent = ui->untrackedFilesList;
         else if (staged)
            parent = ui->stagedFilesList;

         auto item = new QListWidgetItem(fileList);
         item->setData(GitQlientRole::U_ListRole, QVariant::fromValue(parent));
         item->setData(GitQlientRole::U_Name, fileName);

         if (fileList == ui->stagedFilesList)
            item->setFlags(item->flags() & (~Qt::ItemIsSelectable & ~Qt::ItemIsEnabled));

         mCurrentFilesCache.insert(fileName, qMakePair(true, item));

         if (isConflict)
         {
            item->setData(GitQlientRole::U_IsConflict, isConflict);

            const auto newName = fileName + QString(" (conflicts");
            item->setText(newName);
            item->setData(GitQlientRole::U_Name, newName);
         }
         else
            item->setText(fileName);

         item->setToolTip(fileName);

         const auto fileWidget = new FileWidget(
             !untrackedFile && staged ? QString(":/icons/remove") : QString(":/icons/add"), item->text());
         const auto textColor = getColorForFile(files, i);

         fileWidget->setTextColor(textColor);
         item->setForeground(textColor);

         if (staged)
            connect(fileWidget, &FileWidget::clicked, this, [this, item]() { resetFile(item); });
         else
            connect(fileWidget, &FileWidget::clicked, this, [this, item]() { addFileToCommitList(item); });

         fileList->setItemWidget(item, fileWidget);
         item->setText("");
         item->setSizeHint(fileWidget->sizeHint());
      }
      else
         mCurrentFilesCache[fileName].first = true;
   }
}

void AmendWidget::addAllFilesToCommitList()
{
   for (auto i = ui->unstagedFilesList->count() - 1; i >= 0; --i)
      addFileToCommitList(ui->unstagedFilesList->item(i));

   ui->lUnstagedCount->setText(QString("(%1)").arg(ui->unstagedFilesList->count()));
   ui->lStagedCount->setText(QString("(%1)").arg(ui->stagedFilesList->count()));
   ui->pbCommit->setEnabled(ui->stagedFilesList->count() > 0);
}

void AmendWidget::requestDiff(const QString &fileName)
{
   emit signalShowDiff(CommitInfo::ZERO_SHA, mCurrentSha, fileName);
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
   auto needsUpdate = false;

   for (auto i = ui->unstagedFilesList->count() - 1; i >= 0; --i)
   {
      QScopedPointer<GitLocal> git(new GitLocal(mGit));
      needsUpdate |= git->checkoutFile(ui->unstagedFilesList->takeItem(i)->toolTip());
   }

   if (needsUpdate)
      emit signalCheckoutPerformed();
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
   for (const auto &pair : mCurrentFilesCache.values())
      if (pair.second->data(GitQlientRole::U_IsConflict).toBool())
         return true;

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
      {
         QMessageBox::warning(this, tr("Impossible to commit"),
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
