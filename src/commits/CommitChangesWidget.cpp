#include <CommitChangesWidget.h>

#include "ui_CommitChangesWidget.h"

#include <ClickableFrame.h>
#include <CommitInfo.h>
#include <FileWidget.h>
#include <GitBase.h>
#include <GitCache.h>
#include <GitConfig.h>
#include <GitHistory.h>
#include <GitLocal.h>
#include <GitQlientSettings.h>
#include <GitQlientStyles.h>
#include <GitRepoLoader.h>
#include <GitWip.h>
#include <RevisionFiles.h>
#include <UnstagedMenu.h>
#include <WipHelper.h>

#include <QCheckBox>
#include <QDir>
#include <QItemDelegate>
#include <QKeyEvent>
#include <QListWidgetItem>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QProcess>
#include <QRegularExpression>
#include <QScrollBar>
#include <QTextStream>
#include <QToolTip>

#include <QLogger.h>

using namespace QLogger;

enum GitQlientRole
{
   U_IsConflict = Qt::UserRole,
   U_FullPath
};

CommitChangesWidget::CommitChangesWidget(const QSharedPointer<GitCache> &cache, const QSharedPointer<GitBase> &git,
                                         QWidget *parent)
   : QWidget(parent)
   , ui(new Ui::CommitChangesWidget)
   , mCache(cache)
   , mGit(git)
{
   ui->setupUi(this);
   setAttribute(Qt::WA_DeleteOnClose);

   ui->amendFrame->setVisible(false);

   mTitleMaxLength = GitQlientSettings().globalValue("commitTitleMaxLength", mTitleMaxLength).toInt();
   ui->lCounter->setText(QString::number(mTitleMaxLength));
   ui->leCommitTitle->setMaxLength(mTitleMaxLength);
   ui->teDescription->setMaximumHeight(100);

   ui->unstagedFrame->setTitle(tr("Unstaged files"));
   ui->unstagedFrame->setExpandable(true);
   ui->unstagedFrame->setExpanded(true);

   ui->stagedFrame->setTitle(tr("Staged files"));
   ui->stagedFrame->setExpandable(true);
   ui->stagedFrame->setExpanded(true);

   mUnstagedFilesList = new QListWidget();
   mUnstagedFilesList->setContextMenuPolicy(Qt::CustomContextMenu);
   mUnstagedFilesList->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
   mUnstagedFilesList->setObjectName("unstaged");
   ui->unstagedFrame->setContentWidget(mUnstagedFilesList);

   mStagedFilesList = new QListWidget();
   mStagedFilesList->setContextMenuPolicy(Qt::CustomContextMenu);
   mStagedFilesList->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
   mStagedFilesList->setObjectName("staged");
   ui->stagedFrame->setContentWidget(mStagedFilesList);

   connect(ui->amendCheckBox, &QCheckBox::toggled, this, &CommitChangesWidget::onAmendToggled);
   connect(ui->leCommitTitle, &QLineEdit::textChanged, this, &CommitChangesWidget::updateCounter);
   connect(ui->leCommitTitle, &QLineEdit::returnPressed, this, &CommitChangesWidget::commitChanges);
   connect(ui->applyActionBtn, &QPushButton::clicked, this, &CommitChangesWidget::commitChanges);

   connect(mUnstagedFilesList, &QListWidget::customContextMenuRequested, this,
           &CommitChangesWidget::showUnstagedMenu);

   connect(mStagedFilesList, &QListWidget::itemClicked, this,
           [this](QListWidgetItem *item) {
              handleItemClick(mStagedFilesList, item);
           });

   connect(mUnstagedFilesList, &QListWidget::itemClicked, this,
           [this](QListWidgetItem *item) {
              handleItemClick(mUnstagedFilesList, item);
           });

   ui->applyActionBtn->setText(tr("Commit"));
}

CommitChangesWidget::~CommitChangesWidget()
{
   delete ui;
}

void CommitChangesWidget::setCommitMode(CommitMode mode)
{
   mCommitMode = mode;

   const bool isAmend = (mode == CommitMode::Amend);
   ui->amendCheckBox->setChecked(isAmend);
   ui->amendFrame->setVisible(isAmend);
   ui->applyActionBtn->setText(isAmend ? tr("Amend") : tr("Commit"));
}

void CommitChangesWidget::onAmendToggled(bool checked)
{
   if (checked)
   {
      mCommitMode = CommitMode::Amend;
      ui->amendFrame->setVisible(true);
      ui->applyActionBtn->setText(tr("Amend"));

      const auto lastCommitSha = mGit->getLastCommit().output.trimmed();
      configureAmendMode(lastCommitSha);
   }
   else
   {
      mCommitMode = CommitMode::Wip;
      ui->amendFrame->setVisible(false);
      ui->applyActionBtn->setText(tr("Commit"));

      ui->leAuthorName->clear();
      ui->leAuthorEmail->clear();

      configureWipMode(ZERO_SHA);
   }
}

void CommitChangesWidget::configure(const QString &sha)
{
   if (mCommitMode == CommitMode::Amend)
   {
      configureAmendMode(sha);
   }
   else
   {
      configureWipMode(sha);
   }
}

void CommitChangesWidget::configureWipMode(const QString &sha)
{
   mCurrentSha = ZERO_SHA;
   const auto commit = mCache->commitInfo(sha);

   WipHelper::update(mGit, mCache);

   const auto files = mCache->revisionFile(ZERO_SHA, commit.firstParent());

   QLog_Info("UI", QString("Configuring WIP widget"));

   prepareCache();

   if (files)
      insertFiles(files.value(), mUnstagedFilesList);

   clearCache();

   ui->applyActionBtn->setEnabled(mStagedFilesList->count());
}

void CommitChangesWidget::configureAmendMode(const QString &sha)
{
   const auto commitSha = (sha == ZERO_SHA) ? mGit->getLastCommit().output.trimmed() : sha;
   const auto commit = mCache->commitInfo(commitSha);

   if (commit.parentsCount() <= 0)
      return;

   WipHelper::update(mGit, mCache);

   const auto files = mCache->revisionFile(ZERO_SHA, commitSha);
   auto amendFiles = mCache->revisionFile(commitSha, commit.firstParent());

   if (!amendFiles)
   {
      QScopedPointer<GitHistory> git(new GitHistory(mGit));
      const auto ret = git->getDiffFiles(commitSha, commit.firstParent());

      if (ret.success)
      {
         amendFiles = RevisionFiles(ret.output);
         mCache->insertRevisionFiles(commitSha, commit.firstParent(), amendFiles.value());
      }
   }

   if (mCurrentSha != commitSha)
   {
      QLog_Info("UI", QString("Amending sha {%1}.").arg(commitSha));

      mCurrentSha = commitSha;

      const auto author = commit.author;
      if (author.contains("<") && author.contains(">"))
      {
         const auto nameEnd = author.indexOf('<');
         const auto emailStart = nameEnd + 1;
         const auto emailEnd = author.indexOf('>');

         ui->leAuthorName->setText(author.left(nameEnd).trimmed());
         ui->leAuthorEmail->setText(author.mid(emailStart, emailEnd - emailStart));
      }

      ui->teDescription->setPlainText(commit.longLog.trimmed());
      ui->leCommitTitle->setText(commit.shortLog);

      blockSignals(true);
      mUnstagedFilesList->clear();
      mStagedFilesList->clear();
      mInternalCache.clear();
      blockSignals(false);

      if (files)
         insertFiles(files.value(), mUnstagedFilesList);

      if (amendFiles)
         insertFiles(amendFiles.value(), mStagedFilesList);
   }
   else
   {
      QLog_Info("UI", QString("Updating files for SHA {%1}").arg(mCurrentSha));

      prepareCache();

      if (files)
         insertFiles(files.value(), mUnstagedFilesList);

      clearCache();

      if (amendFiles)
         insertFiles(amendFiles.value(), mStagedFilesList);
   }

   ui->applyActionBtn->setEnabled(mStagedFilesList->count());
}

void CommitChangesWidget::commitChanges()
{
   if (mCommitMode == CommitMode::Amend)
   {
      commitAmendChanges();
   }
   else
   {
      commitWipChanges();
   }
}

void CommitChangesWidget::commitWipChanges()
{
   QString msg;
   QStringList selFiles = getFiles();

   if (!selFiles.isEmpty())
   {
      if (hasConflicts())
         QMessageBox::warning(this, tr("Impossible to commit"),
                              tr("There are files with conflicts. Please, resolve "
                                 "the conflicts first."));
      else if (checkMsg(msg))
      {
         const auto revInfo = mCache->commitInfo(ZERO_SHA);

         WipHelper::update(mGit, mCache);

         if (const auto files = mCache->revisionFile(ZERO_SHA, revInfo.firstParent()); files)
         {
            const auto lastShaBeforeCommit = mGit->getLastCommit().output.trimmed();
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            QScopedPointer<GitLocal> gitLocal(new GitLocal(mGit));
            const auto ret = gitLocal->commitFiles(selFiles, files.value(), msg);
            QApplication::restoreOverrideCursor();

            if (ret.success)
            {
               const auto currentSha = mGit->getLastCommit().output.trimmed();
               QScopedPointer<GitConfig> gitConfig(new GitConfig(mGit));
               auto committer = gitConfig->getLocalUserInfo();

               if (committer.mUserEmail.isEmpty() || committer.mUserName.isEmpty())
                  committer = gitConfig->getGlobalUserInfo();

               const auto message = msg.split("\n\n");

               CommitInfo newCommit { currentSha,
                                      { lastShaBeforeCommit },
                                      std::chrono::seconds(QDateTime::currentDateTime().toSecsSinceEpoch()),
                                      ui->leCommitTitle->text() };

               newCommit.committer = QString("%1<%2>").arg(committer.mUserName, committer.mUserEmail);
               newCommit.author = QString("%1<%2>").arg(committer.mUserName, committer.mUserEmail);
               newCommit.longLog = ui->teDescription->toPlainText();

               mCache->insertCommit(newCommit);
               mCache->deleteReference(lastShaBeforeCommit, References::Type::LocalBranch, mGit->getCurrentBranch());
               mCache->insertReference(currentSha, References::Type::LocalBranch, mGit->getCurrentBranch());

               QScopedPointer<GitHistory> gitHistory(new GitHistory(mGit));
               const auto ret = gitHistory->getDiffFiles(currentSha, lastShaBeforeCommit);

               mCache->insertRevisionFiles(currentSha, lastShaBeforeCommit, RevisionFiles(ret.output));

               prepareCache();
               clearCache();

               mStagedFilesList->clear();
               mStagedFilesList->update();

               ui->leCommitTitle->clear();
               ui->teDescription->clear();

               WipHelper::update(mGit, mCache);

               emit mCache->signalCacheUpdated();
               emit changesCommitted();
            }
            else
            {
               QMessageBox msgBox(QMessageBox::Critical, tr("Error when committing"),
                                  tr("There were problems during the commit "
                                     "operation. Please, see the detailed "
                                     "description for more information."),
                                  QMessageBox::Ok, this);
               msgBox.setDetailedText(ret.output);
               msgBox.setStyleSheet(GitQlientStyles::getStyles());
               msgBox.exec();
            }
         }
      }
   }
}

void CommitChangesWidget::commitAmendChanges()
{
   QStringList selFiles = getFiles();

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
         WipHelper::update(mGit, mCache);

         const auto files = mCache->revisionFile(ZERO_SHA, mCurrentSha);

         if (files)
         {
            const auto author = QString("%1<%2>").arg(ui->leAuthorName->text(), ui->leAuthorEmail->text());
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

            QScopedPointer<GitLocal> gitLocal(new GitLocal(mGit));
            const auto ret = gitLocal->amendCommit(selFiles, files.value(), msg, author);
            QApplication::restoreOverrideCursor();

            emit logReload();

            if (ret.success)
            {
               const auto newSha = mGit->getLastCommit().output.trimmed();
               auto commit = mCache->commitInfo(mCurrentSha);
               const auto oldSha = commit.sha;
               commit.sha = newSha;
               commit.committer = author;
               commit.author = author;

               const auto log = msg.split("\n\n");
               commit.shortLog = log.constFirst();
               commit.longLog = log.constLast();

               mCache->updateCommit(oldSha, std::move(commit));

               QScopedPointer<GitHistory> git(new GitHistory(mGit));
               const auto ret = git->getDiffFiles(mCurrentSha, commit.firstParent());

               mCache->insertRevisionFiles(mCurrentSha, commit.firstParent(), RevisionFiles(ret.output));

               emit changesCommitted();
            }
            else
            {
               QMessageBox msgBox(QMessageBox::Critical, tr("Error when amending"),
                                  tr("There were problems during the commit "
                                     "operation. Please, see the detailed "
                                     "description for more information."),
                                  QMessageBox::Ok, this);
               msgBox.setDetailedText(ret.output);
               msgBox.setStyleSheet(GitQlientStyles::getStyles());
               msgBox.exec();
            }
         }
      }
   }
}

void CommitChangesWidget::reload()
{
   configure(mCurrentSha);
}

void CommitChangesWidget::resetFile(QListWidgetItem *item)
{
   const auto fileName = item->toolTip();
   const auto revInfo = mCache->commitInfo(mCurrentSha);
   const auto files = mCache->revisionFile(mCurrentSha, revInfo.firstParent());

   for (auto i = 0; i < files->count(); ++i)
   {
      auto fileName = files->getFile(i);

      if (fileName == item->toolTip())
      {
         const auto isUnknown = files->statusCmp(i, RevisionFiles::UNKNOWN);
         const auto isInIndex = files->statusCmp(i, RevisionFiles::IN_INDEX);
         const auto untrackedFile = !isInIndex && isUnknown;
         QFontMetrics metrix(item->font());

         if (isInIndex || untrackedFile)
         {
            mStagedFilesList->takeItem(mStagedFilesList->row(item));
            mStagedFilesList->repaint();

            const auto clippedText = metrix.elidedText(item->toolTip(), Qt::ElideMiddle, width() - 10);
            const auto newFileWidget = new FileWidget(":/icons/add", clippedText, this);
            newFileWidget->setToolTip(fileName);

            connect(newFileWidget, &FileWidget::clicked, this, [this, item]() { addFileToCommitList(item); });

            mUnstagedFilesList->addItem(item);
            mUnstagedFilesList->setItemWidget(item, newFileWidget);
            mUnstagedFilesList->repaint();

            ui->applyActionBtn->setDisabled(mStagedFilesList->count() == 0);

            delete mStagedFilesList->itemWidget(item);

            break;
         }
      }
   }

   ui->unstagedFrame->setCount(mUnstagedFilesList->count());
   ui->stagedFrame->setCount(mStagedFilesList->count());

   QScopedPointer<GitLocal> git(new GitLocal(mGit));
   if (const auto ret = git->resetFile(item->toolTip()); ret.success)
      emit signalUpdateWip();
}

QColor CommitChangesWidget::getColorForFile(const RevisionFiles &files, int index) const
{
   const auto isUnknown = files.statusCmp(index, RevisionFiles::UNKNOWN);
   const auto isInIndex = files.statusCmp(index, RevisionFiles::IN_INDEX);
   const auto untrackedFile = !isInIndex && isUnknown;
   const auto isConflict = files.statusCmp(index, RevisionFiles::CONFLICT);
   const auto isPartiallyCached = files.statusCmp(index, RevisionFiles::PARTIALLY_CACHED);

   QColor myColor;
   const auto isDeleted = files.statusCmp(index, RevisionFiles::DELETED);

   if (isConflict)
      myColor = GitQlientStyles::getBlue();
   else if (isDeleted)
      myColor = GitQlientStyles::getRed();
   else if (untrackedFile)
      myColor = GitQlientStyles::getOrange();
   else if (files.statusCmp(index, RevisionFiles::NEW) || isUnknown || isInIndex || isPartiallyCached)
      myColor = GitQlientStyles::getGreen();
   else
      myColor = GitQlientStyles::getTextColor();

   return myColor;
}

void CommitChangesWidget::prepareCache()
{
   for (auto it = mInternalCache.begin(); it != mInternalCache.end(); ++it)
      it.value().keep = false;
}

void CommitChangesWidget::clearCache()
{

   for (auto it = mInternalCache.begin(); it != mInternalCache.end();)
   {
      if (!it.value().keep)
      {
         if (it.value().item)
            delete it.value().item;

         it = mInternalCache.erase(it);
      }
      else
         ++it;
   }
}

void CommitChangesWidget::insertFiles(const RevisionFiles &files, QListWidget *fileList)
{
   for (auto &cachedItem : mInternalCache)
      cachedItem.keep = false;

   for (auto i = 0; i < files.count(); ++i)
   {
      const auto fileName = files.getFile(i);
      const auto isUnknown = files.statusCmp(i, RevisionFiles::UNKNOWN);
      const auto isInIndex = files.statusCmp(i, RevisionFiles::IN_INDEX);
      const auto isConflict = files.statusCmp(i, RevisionFiles::CONFLICT);
      const auto isPartiallyCached = files.statusCmp(i, RevisionFiles::PARTIALLY_CACHED);
      const auto staged = isInIndex && !isUnknown && !isConflict;
      auto wip = QString("%1-%2").arg(fileName, mStagedFilesList->objectName());

      if (staged || isPartiallyCached)
      {
         if (!mInternalCache.contains(wip))
         {
            const auto color = getColorForFile(files, i);
            const auto itemPair
                = fillFileItemInfo(fileName, isConflict, QString(":/icons/remove"), color, mStagedFilesList);
            connect(itemPair.second, &FileWidget::clicked, this, [this, item = itemPair.first]() { resetFile(item); });

            mStagedFilesList->setItemWidget(itemPair.first, itemPair.second);
            itemPair.first->setSizeHint(itemPair.second->sizeHint());
         }
         else
            mInternalCache[wip].keep = true;
      }

      if (!staged)
      {
         wip = QString("%1-%2").arg(fileName, fileList->objectName());
         if (!mInternalCache.contains(wip))
         {
            auto color = getColorForFile(files, i);

            if (!files.statusCmp(i, RevisionFiles::NEW) && color == GitQlientStyles::getGreen())
               color = GitQlientStyles::getTextColor();

            const auto itemPair = fillFileItemInfo(fileName, isConflict, QString(":/icons/add"), color, fileList);

            connect(itemPair.second, &FileWidget::clicked, this,
                    [this, item = itemPair.first]() { addFileToCommitList(item); });

            fileList->setItemWidget(itemPair.first, itemPair.second);
            itemPair.first->setSizeHint(itemPair.second->sizeHint());
         }
         else
            mInternalCache[wip].keep = true;
      }
   }

   ui->unstagedFrame->setCount(mUnstagedFilesList->count());
   ui->stagedFrame->setCount(mStagedFilesList->count());
}

QPair<QListWidgetItem *, FileWidget *> CommitChangesWidget::fillFileItemInfo(const QString &file, bool isConflict,
                                                                             const QString &icon, const QColor &color,
                                                                             QListWidget *parent)
{
   auto modName = file;
   auto item = new QListWidgetItem(parent);

   item->setData(GitQlientRole::U_FullPath, file);

   if (isConflict)
   {
      modName = QString(file + " (conflicts)");

      item->setData(GitQlientRole::U_IsConflict, isConflict);
   }

   item->setToolTip(modName);

   const auto clippedText = QFontMetrics(item->font()).elidedText(modName, Qt::ElideMiddle, width() - 10);
   const auto fileWidget = new FileWidget(icon, clippedText, this);
   fileWidget->setTextColor(color);
   fileWidget->setToolTip(modName);

   mInternalCache.insert(QString("%1-%2").arg(file, parent->objectName()), { true, item });

   return qMakePair(item, fileWidget);
}

void CommitChangesWidget::addAllFilesToCommitList()
{
   QStringList files;

   for (auto i = mUnstagedFilesList->count() - 1; i >= 0; --i)
      files += addFileToCommitList(mUnstagedFilesList->item(i), false);

   const auto git = QScopedPointer<GitLocal>(new GitLocal(mGit));

   if (const auto ret = git->markFilesAsResolved(files); ret.success)
      WipHelper::update(mGit, mCache);

   ui->applyActionBtn->setEnabled(mStagedFilesList->count() > 0);
}

void CommitChangesWidget::requestDiff(const QString &fileName)
{
   const auto isCached = qobject_cast<QListWidget *>(sender()) == mStagedFilesList;
   emit signalShowDiff(fileName, isCached);
}

QString CommitChangesWidget::addFileToCommitList(QListWidgetItem *item, bool updateGit)
{
   const auto fileWidget = qobject_cast<FileWidget *>(mUnstagedFilesList->itemWidget(item));
   const auto fileName = fileWidget->toolTip().remove(tr("(conflicts)")).trimmed();
   const auto textColor = fileWidget->getTextColor();

   mUnstagedFilesList->removeItemWidget(item);
   mUnstagedFilesList->takeItem(mUnstagedFilesList->row(item));
   mUnstagedFilesList->repaint();
   delete fileWidget;

   const auto newKey = QString("%1-%2").arg(fileName, mStagedFilesList->objectName());

   if (!mInternalCache.contains(newKey))
   {
      const auto wip = mInternalCache.take(QString("%1-%2").arg(fileName, mUnstagedFilesList->objectName()));
      mInternalCache.insert(newKey, wip);

      QFontMetrics metrix(item->font());
      const auto clippedText = metrix.elidedText(fileName, Qt::ElideMiddle, width() - 10);

      const auto newFileWidget = new FileWidget(":/icons/remove", clippedText, this);
      newFileWidget->setTextColor(textColor);
      newFileWidget->setToolTip(fileName);

      connect(newFileWidget, &FileWidget::clicked, this, [this, item]() { removeFileFromCommitList(item); });

      mStagedFilesList->addItem(item);
      mStagedFilesList->setItemWidget(item, newFileWidget);
      mStagedFilesList->repaint();

      if (item->data(GitQlientRole::U_IsConflict).toBool())
         newFileWidget->setText(fileName);
   }

   ui->applyActionBtn->setEnabled(true);

   if (updateGit)
   {
      const auto git = QScopedPointer<GitLocal>(new GitLocal(mGit));

      if (const auto ret = git->stageFile(fileName); ret.success)
         WipHelper::update(mGit, mCache);
   }

   ui->unstagedFrame->setCount(mUnstagedFilesList->count());
   ui->stagedFrame->setCount(mStagedFilesList->count());

   emit fileStaged(fileName);

   return fileName;
}

void CommitChangesWidget::revertAllChanges()
{
   auto needsUpdate = false;

   for (auto i = mUnstagedFilesList->count() - 1; i >= 0; --i)
   {
      QScopedPointer<GitLocal> git(new GitLocal(mGit));
      needsUpdate |= git->checkoutFile(mUnstagedFilesList->takeItem(i)->toolTip());
   }

   if (needsUpdate)
      emit unstagedFilesChanged();
}

void CommitChangesWidget::removeFileFromCommitList(QListWidgetItem *item)
{
   if (item->flags() & Qt::ItemIsSelectable)
   {
      const auto row = mStagedFilesList->row(item);
      const auto fileWidget = qobject_cast<FileWidget *>(mStagedFilesList->itemWidget(item));
      const auto fileName = fileWidget->toolTip();
      const auto clippedText = QFontMetrics(item->font()).elidedText(fileName, Qt::ElideMiddle, width() - 10);
      const auto wip = mInternalCache.take(QString("%1-%2").arg(fileName, mStagedFilesList->objectName()));

      mInternalCache.insert(QString("%1-%2").arg(fileName, mUnstagedFilesList->objectName()), wip);

      mStagedFilesList->removeItemWidget(item);
      mStagedFilesList->takeItem(row);
      mStagedFilesList->repaint();

      const auto newFileWidget = new FileWidget(":/icons/add", clippedText, this);
      newFileWidget->setTextColor(fileWidget->getTextColor());
      newFileWidget->setToolTip(fileName);

      connect(newFileWidget, &FileWidget::clicked, this, [this, item]() { addFileToCommitList(item); });

      if (item->data(GitQlientRole::U_IsConflict).toBool())
         newFileWidget->setText(fileName + tr(" (conflicts)"));

      mUnstagedFilesList->addItem(item);
      mUnstagedFilesList->setItemWidget(item, newFileWidget);
      mUnstagedFilesList->repaint();

      ui->applyActionBtn->setDisabled(mStagedFilesList->count() == 0);

      delete fileWidget;

      QScopedPointer<GitLocal> git(new GitLocal(mGit));
      if (const auto ret = git->resetFile(item->toolTip()); ret.success)
         emit signalUpdateWip();

      ui->unstagedFrame->setCount(mUnstagedFilesList->count());
      ui->stagedFrame->setCount(mStagedFilesList->count());
   }
}

QStringList CommitChangesWidget::getFiles()
{
   QStringList selFiles;
   const auto totalItems = mStagedFilesList->count();

   for (auto i = 0; i < totalItems; ++i)
   {
      const auto fileWidget = mStagedFilesList->itemWidget(mStagedFilesList->item(i));
      selFiles.append(fileWidget->toolTip());
   }

   return selFiles;
}

bool CommitChangesWidget::checkMsg(QString &msg)
{
   const auto title = ui->leCommitTitle->text().trimmed();

   if (title.isEmpty())
   {
      QMessageBox::warning(this, "Commit changes", "Please, add a title.");
      return false;
   }

   msg = title;

   if (!ui->teDescription->toPlainText().isEmpty())
   {
      auto description = QString("\n\n%1").arg(ui->teDescription->toPlainText());
      description.remove(QRegularExpression("(^|\\n)\\s*#[^\\n]*"));
      msg += description;
   }

   msg.replace(QRegularExpression("[ \\t\\r\\f\\v]+\\n"), "\n");
   msg = msg.trimmed();

   if (msg.isEmpty())
   {
      QMessageBox::warning(this, "Commit changes", "Please, add a title.");
      return false;
   }

   msg = QString("%1\n%2\n")
             .arg(msg.section('\n', 0, 0, QString::SectionIncludeTrailingSep), msg.section('\n', 1).trimmed());

   msg.replace("'", "\'");
   msg.replace('"', '\"');

   return true;
}

void CommitChangesWidget::updateCounter(const QString &text)
{
   ui->lCounter->setText(QString::number(mTitleMaxLength - text.length()));
}

bool CommitChangesWidget::hasConflicts()
{
   for (const auto &iter : std::as_const(mInternalCache))
      if (iter.item->data(GitQlientRole::U_IsConflict).toBool())
         return true;

   return false;
}

void CommitChangesWidget::clear()
{
   mUnstagedFilesList->clear();
   mStagedFilesList->clear();
   mInternalCache.clear();
   ui->leCommitTitle->clear();
   ui->teDescription->clear();
   ui->applyActionBtn->setEnabled(false);

   mLastSelectedItem = nullptr;
   mLastSelectedList = nullptr;

   ui->unstagedFrame->setCount(0);
   ui->stagedFrame->setCount(0);
}

void CommitChangesWidget::clearStaged()
{
   if (mLastSelectedList == mStagedFilesList)
   {
      mLastSelectedItem = nullptr;
      mLastSelectedList = nullptr;
   }

   mStagedFilesList->clear();

   const auto end = mInternalCache.end();
   for (auto it = mInternalCache.begin(); it != end;)
   {
      if (it.key().contains(QString("-%1").arg(mStagedFilesList->objectName())))
         it = mInternalCache.erase(it);
      else
         ++it;
   }

   ui->applyActionBtn->setEnabled(false);

   ui->stagedFrame->setCount(0);
}

void CommitChangesWidget::setCommitTitleMaxLength()
{
   mTitleMaxLength = GitQlientSettings().globalValue("commitTitleMaxLength", mTitleMaxLength).toInt();

   ui->lCounter->setText(QString::number(mTitleMaxLength));
   ui->leCommitTitle->setMaxLength(mTitleMaxLength);
   updateCounter(ui->leCommitTitle->text());
}

void CommitChangesWidget::showUnstagedMenu(const QPoint &pos)
{
   const auto item = mUnstagedFilesList->itemAt(pos);

   if (item)
   {
      const auto contextMenu = new UnstagedMenu(mGit, item->toolTip(), this);
      connect(contextMenu, &UnstagedMenu::signalShowDiff, this, &CommitChangesWidget::requestDiff);
      connect(contextMenu, &UnstagedMenu::signalCommitAll, this, &CommitChangesWidget::addAllFilesToCommitList);
      connect(contextMenu, &UnstagedMenu::signalRevertAll, this, &CommitChangesWidget::revertAllChanges);
      connect(contextMenu, &UnstagedMenu::changeReverted, this, &CommitChangesWidget::changeReverted);
      connect(contextMenu, &UnstagedMenu::signalCheckedOut, this, &CommitChangesWidget::unstagedFilesChanged);
      connect(contextMenu, &UnstagedMenu::signalShowFileHistory, this, &CommitChangesWidget::signalShowFileHistory);
      connect(contextMenu, &UnstagedMenu::signalStageFile, this, [this, item] { addFileToCommitList(item); });
      connect(contextMenu, &UnstagedMenu::untrackedDeleted, this, &CommitChangesWidget::unstagedFilesChanged);

      const auto parentPos = mUnstagedFilesList->mapToParent(pos);
      contextMenu->popup(mapToGlobal(parentPos));
   }
}

void CommitChangesWidget::handleItemClick(QListWidget *list, QListWidgetItem *item)
{
   if (mLastSelectedItem == item && mLastSelectedList == list && item->isSelected())
   {
      list->clearSelection();
      mLastSelectedItem = nullptr;
      mLastSelectedList = nullptr;

      emit signalReturnToHistory();
   }
   else
   {
      if (list == mStagedFilesList)
         mUnstagedFilesList->clearSelection();
      else
         mStagedFilesList->clearSelection();

      item->setSelected(true);
      mLastSelectedItem = item;
      mLastSelectedList = list;

      requestDiff(mGit->getWorkingDir() + "/" + item->toolTip());
   }
}
