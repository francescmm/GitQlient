#include <CommitChangesWidget.h>
#include <ui_CommitChangesWidget.h>

#include <GitRepoLoader.h>
#include <GitBase.h>
#include <GitLocal.h>
#include <GitQlientStyles.h>
#include <CommitInfo.h>
#include <RevisionFiles.h>
#include <UnstagedMenu.h>
#include <GitCache.h>
#include <FileWidget.h>
#include <UntrackedMenu.h>

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

const int CommitChangesWidget::kMaxTitleChars = 50;

QString CommitChangesWidget::lastMsgBeforeError;

enum GitQlientRole
{
   U_ListRole = Qt::UserRole,
   U_IsConflict,
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

   ui->lCounter->setText(QString::number(kMaxTitleChars));
   ui->leCommitTitle->setMaxLength(kMaxTitleChars);
   ui->teDescription->setMaximumHeight(125);

   QIcon stagedIcon(":/icons/staged");
   ui->stagedFilesIcon->setPixmap(stagedIcon.pixmap(15, 15));

   QIcon unstagedIcon(":/icons/unstaged");
   ui->unstagedIcon->setPixmap(unstagedIcon.pixmap(15, 15));

   QIcon untrackedIcon(":/icons/untracked");
   ui->untrackedFilesIcon->setPixmap(untrackedIcon.pixmap(15, 15));

   connect(ui->leCommitTitle, &QLineEdit::textChanged, this, &CommitChangesWidget::updateCounter);
   connect(ui->leCommitTitle, &QLineEdit::returnPressed, this, &CommitChangesWidget::commitChanges);
   connect(ui->pbCommit, &QPushButton::clicked, this, &CommitChangesWidget::commitChanges);
   connect(ui->pbCancelAmend, &QPushButton::clicked, this, [this]() { emit signalCancelAmend(mCurrentSha); });
   connect(ui->untrackedFilesList, &QListWidget::itemDoubleClicked, this,
           [this](QListWidgetItem *item) { requestDiff(mGit->getWorkingDir() + "/" + item->toolTip()); });
   connect(ui->untrackedFilesList, &QListWidget::customContextMenuRequested, this,
           &CommitChangesWidget::showUntrackedMenu);
   connect(ui->stagedFilesList, &StagedFilesList::signalResetFile, this, &CommitChangesWidget::resetFile);
   connect(ui->stagedFilesList, &StagedFilesList::signalShowDiff, this,
           [this](const QString &fileName) { requestDiff(mGit->getWorkingDir() + "/" + fileName); });
   connect(ui->unstagedFilesList, &QListWidget::customContextMenuRequested, this,
           &CommitChangesWidget::showUnstagedMenu);
   connect(ui->unstagedFilesList, &QListWidget::itemDoubleClicked, this,
           [this](QListWidgetItem *item) { requestDiff(mGit->getWorkingDir() + "/" + item->toolTip()); });

   ui->pbCancelAmend->setVisible(false);
   ui->pbCommit->setText(tr("Commit"));
}

CommitChangesWidget::~CommitChangesWidget()
{
   delete ui;
}

void CommitChangesWidget::reload()
{
   configure(mCurrentSha);
}

void CommitChangesWidget::resetFile(QListWidgetItem *item)
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
         const auto fileWidget = qobject_cast<FileWidget *>(ui->stagedFilesList->itemWidget(item));

         if (isInIndex)
         {
            item->setData(GitQlientRole::U_ListRole, QVariant::fromValue(ui->unstagedFilesList));

            ui->stagedFilesList->takeItem(row);
            ui->unstagedFilesList->addItem(item);

            const auto newFileWidget = new FileWidget(iconPath, item->toolTip());
            newFileWidget->setTextColor(fileWidget->getTextColor());

            connect(newFileWidget, &FileWidget::clicked, this, [this, item]() { addFileToCommitList(item); });
            ui->unstagedFilesList->setItemWidget(item, newFileWidget);

            delete fileWidget;
         }
         else if (untrackedFile)
         {
            item->setData(GitQlientRole::U_ListRole, QVariant::fromValue(ui->untrackedFilesList));

            ui->stagedFilesList->takeItem(row);
            ui->untrackedFilesList->addItem(item);

            const auto newFileWidget = new FileWidget(iconPath, item->toolTip());
            newFileWidget->setTextColor(fileWidget->getTextColor());

            connect(newFileWidget, &FileWidget::clicked, this, [this, item]() { addFileToCommitList(item); });
            ui->untrackedFilesList->setItemWidget(item, newFileWidget);

            delete newFileWidget;
         }
      }
   }

   if (ret.success)
      emit signalUpdateWip();
}

QColor CommitChangesWidget::getColorForFile(const RevisionFiles &files, int index) const
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
      const auto untrackedFile = !isInIndex && isUnknown;
      const auto staged = isInIndex && !isUnknown && !isConflict;
      auto wip = QString("%1-%2").arg(fileName, ui->stagedFilesList->objectName());

      if (staged || isPartiallyCached)
      {
         if (!mInternalCache.contains(wip))
         {
            const auto itemPair = fillFileItemInfo(fileName, isConflict, QString(":/icons/remove"),
                                                   GitQlientStyles::getGreen(), ui->stagedFilesList);
            connect(itemPair.second, &FileWidget::clicked, this, [this, item = itemPair.first]() { resetFile(item); });

            ui->stagedFilesList->setItemWidget(itemPair.first, itemPair.second);
            itemPair.first->setSizeHint(itemPair.second->sizeHint());
         }
         else
            mInternalCache[wip].keep = true;
      }

      const auto parent = untrackedFile ? ui->untrackedFilesList : fileList;

      wip = QString("%1-%2").arg(fileName, parent->objectName());

      if (!staged)
      {
         if (!mInternalCache.contains(wip))
         {
            // Item configuration
            auto color = getColorForFile(files, i);

            // If the item is not new but the color is green this is not correct.
            // It means that the file was partially staged so the color backs to default.
            if (!files.statusCmp(i, RevisionFiles::NEW) && color == GitQlientStyles::getGreen())
               color = GitQlientStyles::getTextColor();

            const auto itemPair = fillFileItemInfo(fileName, isConflict, QString(":/icons/add"), color, parent);

            connect(itemPair.second, &FileWidget::clicked, this,
                    [this, item = itemPair.first]() { addFileToCommitList(item); });

            parent->setItemWidget(itemPair.first, itemPair.second);
            itemPair.first->setSizeHint(itemPair.second->sizeHint());
         }
         else
            mInternalCache[wip].keep = true;
      }
   }
}

QPair<QListWidgetItem *, FileWidget *> CommitChangesWidget::fillFileItemInfo(const QString &file, bool isConflict,
                                                                             const QString &icon, const QColor &color,
                                                                             QListWidget *parent)
{
   auto modName = file;
   auto item = new QListWidgetItem(parent);

   if (isConflict)
   {
      modName = QString(file + " (conflicts)");

      item->setData(GitQlientRole::U_IsConflict, isConflict);
   }

   item->setData(GitQlientRole::U_ListRole, QVariant::fromValue(parent));
   item->setToolTip(modName);

   const auto fileWidget = new FileWidget(icon, modName);
   fileWidget->setTextColor(color);

   mInternalCache.insert(QString("%1-%2").arg(file, parent->objectName()), { true, item });

   return qMakePair(item, fileWidget);
}

void CommitChangesWidget::addAllFilesToCommitList()
{
   for (auto i = ui->unstagedFilesList->count() - 1; i >= 0; --i)
      addFileToCommitList(ui->unstagedFilesList->item(i));

   ui->lUnstagedCount->setText(QString("(%1)").arg(ui->unstagedFilesList->count()));
   ui->lStagedCount->setText(QString("(%1)").arg(ui->stagedFilesList->count()));
   ui->pbCommit->setEnabled(ui->stagedFilesList->count() > 0);
}

void CommitChangesWidget::requestDiff(const QString &fileName)
{
   const auto isCached = qobject_cast<StagedFilesList *>(sender()) == ui->stagedFilesList;
   emit signalShowDiff(CommitInfo::ZERO_SHA, mCache->getCommitInfo(CommitInfo::ZERO_SHA).parent(0), fileName, isCached);
}

void CommitChangesWidget::addFileToCommitList(QListWidgetItem *item)
{
   const auto fileList = qvariant_cast<QListWidget *>(item->data(GitQlientRole::U_ListRole));
   const auto row = fileList->row(item);
   const auto fileWidget = qobject_cast<FileWidget *>(fileList->itemWidget(item));
   const auto fileName = fileWidget->text().remove("(conflicts)").trimmed();

   QScopedPointer<GitLocal> git = QScopedPointer<GitLocal>(new GitLocal(mGit));
   connect(git.data(), &GitLocal::signalWipUpdated, this, [this]() {
      QScopedPointer<GitRepoLoader> loader(new GitRepoLoader(mGit, mCache));
      loader->updateWipRevision();
   });

   git->markFileAsResolved(fileName);

   fileList->removeItemWidget(item);
   fileList->takeItem(row);

   const auto wip = mInternalCache.take(QString("%1-%2").arg(fileName, fileList->objectName()));
   const auto newKey = QString("%1-%2").arg(fileName, ui->stagedFilesList->objectName());

   if (!mInternalCache.contains(newKey))
   {
      mInternalCache.insert(newKey, std::move(wip));

      const auto newFileWidget = new FileWidget(":/icons/remove", fileName);
      newFileWidget->setTextColor(fileWidget->getTextColor());

      connect(newFileWidget, &FileWidget::clicked, this, [this, item]() { removeFileFromCommitList(item); });

      ui->stagedFilesList->addItem(item);
      ui->stagedFilesList->setItemWidget(item, newFileWidget);

      if (item->data(GitQlientRole::U_IsConflict).toBool())
      {
         newFileWidget->setText(fileName);
      }
   }

   delete fileWidget;

   ui->lUntrackedCount->setText(QString("(%1)").arg(ui->untrackedFilesList->count()));
   ui->lUnstagedCount->setText(QString("(%1)").arg(ui->unstagedFilesList->count()));
   ui->lStagedCount->setText(QString("(%1)").arg(ui->stagedFilesList->count()));
   ui->pbCommit->setEnabled(true);
}

void CommitChangesWidget::revertAllChanges()
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

void CommitChangesWidget::removeFileFromCommitList(QListWidgetItem *item)
{
   if (item->flags() & Qt::ItemIsSelectable)
   {
      const auto itemOriginalList = qvariant_cast<QListWidget *>(item->data(GitQlientRole::U_ListRole));
      const auto row = ui->stagedFilesList->row(item);
      const auto fileWidget = qobject_cast<FileWidget *>(ui->stagedFilesList->itemWidget(item));
      const auto fileName = fileWidget->text();

      const auto wip = mInternalCache.take(QString("%1-%2").arg(fileName, ui->stagedFilesList->objectName()));
      mInternalCache.insert(QString("%1-%2").arg(fileName, itemOriginalList->objectName()), std::move(wip));

      const auto newFileWidget = new FileWidget(":/icons/add", fileName);
      newFileWidget->setTextColor(fileWidget->getTextColor());

      connect(newFileWidget, &FileWidget::clicked, this, [this, item]() { addFileToCommitList(item); });

      QScopedPointer<GitLocal> git = QScopedPointer<GitLocal>(new GitLocal(mGit));
      git->resetFile(fileName);

      if (item->data(GitQlientRole::U_IsConflict).toBool())
      {
         newFileWidget->setText(fileName + QString::fromUtf8(" (conflicts)"));
      }

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

QStringList CommitChangesWidget::getFiles()
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

bool CommitChangesWidget::checkMsg(QString &msg)
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

void CommitChangesWidget::updateCounter(const QString &text)
{
   ui->lCounter->setText(QString::number(kMaxTitleChars - text.count()));
}

bool CommitChangesWidget::hasConflicts()
{
   const auto end = mInternalCache.cend();
   for (auto iter = mInternalCache.cbegin(); iter != end; ++iter)
      if (iter.value().item->data(GitQlientRole::U_IsConflict).toBool())
         return true;

   return false;
}

void CommitChangesWidget::clear()
{
   ui->untrackedFilesList->clear();
   ui->unstagedFilesList->clear();
   ui->stagedFilesList->clear();
   mInternalCache.clear();
   ui->leCommitTitle->clear();
   ui->teDescription->clear();
   ui->pbCommit->setEnabled(false);
   ui->lStagedCount->setText(QString("(%1)").arg(ui->stagedFilesList->count()));
   ui->lUnstagedCount->setText(QString("(%1)").arg(ui->unstagedFilesList->count()));
   ui->lUntrackedCount->setText(QString("(%1)").arg(ui->untrackedFilesList->count()));
}

void CommitChangesWidget::showUntrackedMenu(const QPoint &pos)
{
   const auto item = ui->untrackedFilesList->itemAt(pos);

   if (item)
   {
      const auto fileName = item->toolTip();
      const auto contextMenu = new UntrackedMenu(mGit, fileName, this);
      connect(contextMenu, &UntrackedMenu::signalStageFile, this, [this, item]() { addFileToCommitList(item); });
      connect(contextMenu, &UntrackedMenu::signalCheckoutPerformed, this,
              &CommitChangesWidget::signalCheckoutPerformed);

      const auto parentPos = ui->untrackedFilesList->mapToParent(pos);
      contextMenu->popup(mapToGlobal(parentPos));
   }
}
