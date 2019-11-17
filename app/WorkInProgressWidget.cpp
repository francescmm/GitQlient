#include <WorkInProgressWidget.h>
#include <ui_WorkInProgressWidget.h>

#include <git.h>
#include <GitQlientStyles.h>
#include <CommitInfo.h>
#include <RevisionFile.h>
#include <UnstagedFilesContextMenu.h>
#include <FileListDelegate.h>

#include <QDir>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QRegExp>
#include <QScrollBar>
#include <QSettings>
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

WorkInProgressWidget::WorkInProgressWidget(QSharedPointer<Git> git, QWidget *parent)
   : QWidget(parent)
   , ui(new Ui::WorkInProgressWidget)
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
   connect(ui->untrackedFilesList, &QListWidget::itemClicked, this, &WorkInProgressWidget::addFileToCommitList);
   connect(ui->untrackedFilesList, &QListWidget::customContextMenuRequested, this,
           &WorkInProgressWidget::showUntrackedMenu);
   connect(ui->unstagedFilesList, &QListWidget::customContextMenuRequested, this,
           &WorkInProgressWidget::showUnstagedMenu);
   connect(ui->stagedFilesList, &QListWidget::customContextMenuRequested, this, &WorkInProgressWidget::showStagedMenu);
   connect(ui->unstagedFilesList, &QListWidget::itemClicked, this, &WorkInProgressWidget::addFileToCommitList);
   connect(ui->stagedFilesList, &QListWidget::itemClicked, this, &WorkInProgressWidget::removeFileFromCommitList);
}

void WorkInProgressWidget::configure(const QString &sha)
{
   const auto shaChange = mCurrentSha != sha;
   mCurrentSha = sha;
   mIsAmend = mCurrentSha != ZERO_SHA;

   QLog_Info("UI",
             QString("Configuring commit widget with sha {%1} as {%2}")
                 .arg(mCurrentSha)
                 .arg(mIsAmend ? QString("amend") : QString("wip")));

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
   if (mIsAmend)
   {
      const auto revInfo = mGit->getCommitInfo(mCurrentSha);
      const auto author = revInfo.author().split("<");
      ui->leAuthorName->setText(author.first());
      ui->leAuthorEmail->setText(author.last().mid(0, author.last().count() - 1));
   }

   const auto files = mGit->getWipFiles();

   if (!shaChange || (mIsAmend && shaChange))
      prepareCache();

   insertFilesInList(files, ui->unstagedFilesList);

   if (!shaChange || (mIsAmend && shaChange))
      clearCache();

   if (mIsAmend)
   {
      const auto amendFiles = mGit->getCommitFiles(mCurrentSha);
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
         logMessage = mGit->getSplitCommitMsg(mCurrentSha);

      msg = logMessage.second.trimmed();

      if (mIsAmend || shaChange)
         ui->leCommitTitle->setText(logMessage.first);
   }
   else
      msg = lastMsgBeforeError;

   if (mIsAmend || shaChange)
   {
      ui->teDescription->setPlainText(msg);
      ui->teDescription->moveCursor(QTextCursor::Start);
   }

   ui->pbCommit->setEnabled(ui->stagedFilesList->count());
}

void WorkInProgressWidget::insertFilesInList(const RevisionFile &files, QListWidget *fileList)
{
   for (auto i = 0; i < files.count(); ++i)
   {
      QColor myColor;

      const auto isUnknown = files.statusCmp(i, RevisionFile::UNKNOWN);
      const auto isInIndex = files.statusCmp(i, RevisionFile::IN_INDEX);
      const auto untrackedFile = !isInIndex && isUnknown;
      const auto staged = isInIndex && !isUnknown;
      const auto isDeleted = files.statusCmp(i, RevisionFile::DELETED);

      if ((files.statusCmp(i, RevisionFile::NEW) || isUnknown || isInIndex) && !untrackedFile && !isDeleted)
         myColor = GitQlientStyles::getGreen();
      else if (isDeleted)
         myColor = GitQlientStyles::getRed();
      else if (untrackedFile)
         myColor = GitQlientStyles::getOrange();
      else
         myColor = GitQlientStyles::getTextColor();

      const auto fileName = mGit->filePath(files, i);

      if (!mCurrentFilesCache.contains(fileName))
      {
         QListWidgetItem *item = nullptr;

         if (untrackedFile)
         {
            item = new QListWidgetItem(ui->untrackedFilesList);
            item->setData(Qt::UserRole, QVariant::fromValue(ui->untrackedFilesList));
         }
         else if (staged)
         {
            item = new QListWidgetItem(ui->stagedFilesList);
            item->setData(Qt::UserRole, QVariant::fromValue(ui->stagedFilesList));
         }
         else
         {
            item = new QListWidgetItem(fileList);
            item->setData(Qt::UserRole, QVariant::fromValue(fileList));
         }

         item->setText(fileName);
         item->setToolTip(fileName);
         item->setForeground(myColor);

         if (mIsAmend && fileList == ui->stagedFilesList)
            item->setFlags(item->flags() & (~Qt::ItemIsSelectable & ~Qt::ItemIsEnabled));

         mCurrentFilesCache.insert(fileName, qMakePair(true, item));
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
   auto i = ui->unstagedFilesList->count();

   for (; i >= 0; --i)
   {
      auto item = ui->unstagedFilesList->takeItem(i);
      ui->stagedFilesList->addItem(item);
   }

   ui->lUnstagedCount->setText(QString("(%1)").arg(ui->unstagedFilesList->count()));
   ui->lStagedCount->setText(QString("(%1)").arg(ui->stagedFilesList->count()));
   ui->pbCommit->setEnabled(ui->stagedFilesList->count() > 0);
}

void WorkInProgressWidget::addFileToCommitList(QListWidgetItem *item)
{
   const auto fileList = dynamic_cast<QListWidget *>(sender());
   const auto row = fileList->row(item);
   fileList->takeItem(row);
   ui->stagedFilesList->addItem(item);
   ui->lUntrackedCount->setText(QString("(%1)").arg(ui->untrackedFilesList->count()));
   ui->lUnstagedCount->setText(QString("(%1)").arg(ui->unstagedFilesList->count()));
   ui->lStagedCount->setText(QString("(%1)").arg(ui->stagedFilesList->count()));
   ui->pbCommit->setEnabled(true);
}

void WorkInProgressWidget::revertAllChanges()
{
   auto i = ui->unstagedFilesList->count();

   for (; ++i >= 0; ++i)
   {
      const auto fileName = ui->unstagedFilesList->takeItem(i)->data(Qt::DisplayRole).toString();
      const auto ret = mGit->resetFile(fileName);

      emit signalCheckoutPerformed(ret);
   }
}

void WorkInProgressWidget::removeFileFromCommitList(QListWidgetItem *item)
{
   if (item->flags() & Qt::ItemIsSelectable)
   {
      const auto itemOriginalList = qvariant_cast<QListWidget *>(item->data(Qt::UserRole));
      const auto row = ui->stagedFilesList->row(item);

      ui->stagedFilesList->takeItem(row);
      itemOriginalList->addItem(item);
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
      const auto contextMenu = new UnstagedFilesContextMenu(mGit, fileName, this);
      connect(contextMenu, &UnstagedFilesContextMenu::signalShowDiff, this, &WorkInProgressWidget::signalShowDiff);
      connect(contextMenu, &UnstagedFilesContextMenu::signalCommitAll, this,
              &WorkInProgressWidget::addAllFilesToCommitList);
      connect(contextMenu, &UnstagedFilesContextMenu::signalRevertAll, this, &WorkInProgressWidget::revertAllChanges);
      connect(contextMenu, &UnstagedFilesContextMenu::signalCheckedOut, this,
              &WorkInProgressWidget::signalCheckoutPerformed);
      connect(contextMenu, &UnstagedFilesContextMenu::signalShowFileHistory, this,
              &WorkInProgressWidget::signalShowFileHistory);

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
      const auto action = menu->addAction("See changes");
      connect(action, &QAction::triggered, this, [this, fileName]() { emit signalShowDiff(fileName); });

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
      selFiles.append(ui->stagedFilesList->item(i)->text());

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

bool WorkInProgressWidget::commitChanges()
{
   QString msg;
   QStringList selFiles = getFiles();
   auto done = false;

   if (!selFiles.isEmpty() && checkMsg(msg))
   {
      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
      const auto ok = mGit->commitFiles(selFiles, msg, false);
      QApplication::restoreOverrideCursor();

      lastMsgBeforeError = (ok ? "" : msg);

      emit signalChangesCommitted(ok);

      done = true;

      ui->leCommitTitle->clear();
      ui->teDescription->clear();
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

      if (checkMsg(msg))
      {
         const auto author = QString("%1<%2>").arg(ui->leAuthorName->text(), ui->leAuthorEmail->text());

         QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
         const auto ok = mGit->commitFiles(selFiles, msg, true, author);
         QApplication::restoreOverrideCursor();

         emit signalChangesCommitted(ok);

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
   ui->leCommitTitle->clear();
   ui->leAuthorName->clear();
   ui->leAuthorEmail->clear();
   ui->teDescription->clear();
   ui->pbCommit->setEnabled(false);
   ui->lStagedCount->setText(QString("(%1)").arg(ui->unstagedFilesList->count()));
   ui->lUnstagedCount->setText(QString("(%1)").arg(ui->stagedFilesList->count()));
   ui->lUntrackedCount->setText(QString("(%1)").arg(ui->unstagedFilesList->count()));
}
