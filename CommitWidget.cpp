/*
        Description: changes commit dialog

Author: Marco Costalba (C) 2005-2007

             Copyright: See COPYING file that comes with this distribution
                             */
#include <CommitWidget.h>
#include <ui_CommitWidget.h>

#include <git.h>
#include <Revision.h>
#include <RevisionFile.h>
#include <UnstagedFilesContextMenu.h>

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

#include <QLogger.h>

using namespace QLogger;

const int CommitWidget::kMaxTitleChars = 80;

namespace
{
bool readFromFile(const QString &fileName, QString &data)
{

   data = "";
   QFile file(fileName);
   if (!file.open(QIODevice::ReadOnly))
      return false;

   QTextStream stream(&file);
   data = stream.readAll();
   file.close();
   return true;
}
}

QString CommitWidget::lastMsgBeforeError;

CommitWidget::CommitWidget(QSharedPointer<Git> git, QWidget *parent)
   : QWidget(parent)
   , ui(new Ui::CommitWidget)
   , mGit(git)
{
   ui->setupUi(this);

   ui->lCounter->setText(QString::number(kMaxTitleChars));
   ui->leCommitTitle->setMaxLength(kMaxTitleChars);
   ui->teDescription->setMaximumHeight(125);

   QIcon stagedIcon(":/icons/staged");
   ui->stagedFilesIcon->setPixmap(stagedIcon.pixmap(15, 15));

   QIcon unstagedIcon(":/icons/unstaged");
   ui->unstagedIcon->setPixmap(unstagedIcon.pixmap(15, 15));

   QIcon untrackedIcon(":/icons/untracked");
   ui->untrackedFilesIcon->setPixmap(untrackedIcon.pixmap(15, 15));

   connect(ui->leCommitTitle, &QLineEdit::textChanged, this, &CommitWidget::updateCounter);
   connect(ui->leCommitTitle, &QLineEdit::returnPressed, this, &CommitWidget::applyChanges);
   connect(ui->pbCommit, &QPushButton::clicked, this, &CommitWidget::applyChanges);
   connect(ui->unstagedFilesList, &QListWidget::customContextMenuRequested, this, &CommitWidget::contextMenuPopup);
   connect(ui->unstagedFilesList, &QListWidget::itemClicked, this, &CommitWidget::addFileToCommitList);
   connect(ui->stagedFilesList, &QListWidget::itemClicked, this, &CommitWidget::removeFileFromCommitList);
}

void CommitWidget::init(const QString &shaToAmmend)
{
   mIsAmmend = shaToAmmend != ZERO_SHA;

   QLog_Info("UI",
             QString("Configuring commit widget with sha {%1} as {%2}")
                 .arg(shaToAmmend)
                 .arg(mIsAmmend ? QString("amend") : QString("wip")));

   blockSignals(true);
   clear();
   ui->leAuthorName->setVisible(mIsAmmend);
   ui->leAuthorEmail->setVisible(mIsAmmend);
   blockSignals(false);

   ui->pbCommit->setText(mIsAmmend ? QString("Amend") : QString("Commit"));

   QString templ(".git/commit-template");
   QString msg;

   QDir d;
   if (d.exists(templ))
      readFromFile(templ, msg);

   // set-up files list
   if (mIsAmmend)
   {
      const auto revInfo = mGit->revLookup(shaToAmmend);
      const auto author = revInfo->author().split("<");
      ui->leAuthorName->setText(author.first());
      ui->leAuthorEmail->setText(author.last().mid(0, author.last().count() - 1));
   }

   const RevisionFile *files = nullptr;

   files = mGit->getFiles(mIsAmmend ? shaToAmmend : ZERO_SHA);

   if (files)
   {
      insertFilesInList(files, mIsAmmend ? ui->stagedFilesList : ui->unstagedFilesList);

      if (mIsAmmend)
      {
         files = mGit->getFiles(ZERO_SHA);
         insertFilesInList(files, ui->unstagedFilesList);
      }
   }

   ui->lUnstagedCount->setText(QString("(%1)").arg(ui->unstagedFilesList->count()));
   ui->lStagedCount->setText(QString("(%1)").arg(ui->stagedFilesList->count()));

   // compute cursor offsets. Take advantage of fixed width font

   if (lastMsgBeforeError.isEmpty())
   {
      QPair<QString, QString> logMessage;

      if (mIsAmmend)
         logMessage = mGit->getSplitCommitMsg(shaToAmmend);

      msg = logMessage.second.trimmed();
      ui->leCommitTitle->setText(logMessage.first);
   }
   else
      msg = lastMsgBeforeError;

   ui->teDescription->setPlainText(msg);
   ui->teDescription->moveCursor(QTextCursor::Start);
   ui->pbCommit->setEnabled(ui->stagedFilesList->count());
}

void CommitWidget::insertFilesInList(const RevisionFile *files, QListWidget *fileList)
{
   for (auto i = 0; i < files->count(); ++i)
   {
      QColor myColor;

      if (files->statusCmp(i, RevisionFile::NEW) || files->statusCmp(i, RevisionFile::UNKNOWN))
         myColor = Qt::darkGreen;
      else if (files->statusCmp(i, RevisionFile::DELETED))
         myColor = Qt::red;
      else
         myColor = Qt::white;

      const auto item = new QListWidgetItem(fileList);
      item->setText(mGit->filePath(*files, i));
      item->setForeground(myColor);

      if (mIsAmmend && fileList == ui->stagedFilesList)
         item->setFlags(item->flags() & (~Qt::ItemIsSelectable & ~Qt::ItemIsEnabled));
   }
}

void CommitWidget::addAllFilesToCommitList()
{
   auto i = ui->unstagedFilesList->count();

   for (; ++i >= 0; ++i)
   {
      auto item = ui->unstagedFilesList->takeItem(i);
      ui->stagedFilesList->addItem(item);
   }

   ui->lUnstagedCount->setText(QString("(%1)").arg(ui->unstagedFilesList->count()));
   ui->lStagedCount->setText(QString("(%1)").arg(ui->stagedFilesList->count()));
   ui->pbCommit->setEnabled(i != ui->unstagedFilesList->count());
}

void CommitWidget::addFileToCommitList(QListWidgetItem *item)
{
   const auto row = ui->unstagedFilesList->row(item);
   ui->unstagedFilesList->takeItem(row);
   ui->stagedFilesList->addItem(item);
   ui->lUnstagedCount->setText(QString("(%1)").arg(ui->unstagedFilesList->count()));
   ui->lStagedCount->setText(QString("(%1)").arg(ui->stagedFilesList->count()));
   ui->pbCommit->setEnabled(true);
}

void CommitWidget::revertAllChanges()
{
   auto i = ui->unstagedFilesList->count();

   for (; ++i >= 0; ++i)
   {
      const auto fileName = ui->unstagedFilesList->takeItem(i)->data(Qt::DisplayRole).toString();
      const auto ret = mGit->resetFile(fileName);

      emit signalCheckoutPerformed(ret);
   }
}

void CommitWidget::removeFileFromCommitList(QListWidgetItem *item)
{
   if (item->flags() & Qt::ItemIsSelectable)
   {
      const auto row = ui->stagedFilesList->row(item);
      ui->stagedFilesList->takeItem(row);
      ui->unstagedFilesList->addItem(item);
      ui->lUnstagedCount->setText(QString("(%1)").arg(ui->unstagedFilesList->count()));
      ui->lStagedCount->setText(QString("(%1)").arg(ui->stagedFilesList->count()));
      ui->pbCommit->setDisabled(ui->stagedFilesList->count() == 0);
   }
}

void CommitWidget::contextMenuPopup(const QPoint &pos)
{
   const auto item = ui->unstagedFilesList->itemAt(pos);

   if (item)
   {
      const auto fileName = ui->unstagedFilesList->itemAt(pos)->data(Qt::DisplayRole).toString();
      const auto contextMenu = new UnstagedFilesContextMenu(mGit, fileName, this);
      connect(contextMenu, &UnstagedFilesContextMenu::signalCommitAll, this, &CommitWidget::addAllFilesToCommitList);
      connect(contextMenu, &UnstagedFilesContextMenu::signalRevertAll, this, &CommitWidget::revertAllChanges);
      connect(contextMenu, &UnstagedFilesContextMenu::signalCheckedOut, this, &CommitWidget::signalCheckoutPerformed);

      contextMenu->popup(mapToGlobal(pos));
   }
}

void CommitWidget::applyChanges()
{
   mIsAmmend ? ammendChanges() : commitChanges();
}

QStringList CommitWidget::getFiles()
{
   QStringList selFiles;
   const auto totalItems = ui->stagedFilesList->count();

   for (auto i = 0; i < totalItems; ++i)
      selFiles.append(ui->stagedFilesList->item(i)->text());

   return selFiles;
}

bool CommitWidget::checkMsg(QString &msg)
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

void CommitWidget::updateCounter(const QString &text)
{
   ui->lCounter->setText(QString::number(kMaxTitleChars - text.count()));
}

bool CommitWidget::commitChanges()
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
   }

   return done;
}

bool CommitWidget::ammendChanges()
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

void CommitWidget::clear()
{
   ui->leAuthorName->clear();
   ui->leAuthorEmail->clear();
   ui->unstagedFilesList->clear();
   ui->leCommitTitle->clear();
   ui->teDescription->clear();
   ui->stagedFilesList->clear();
   ui->pbCommit->setEnabled(false);
   ui->lUnstagedCount->setText(QString("(%1)").arg(ui->unstagedFilesList->count()));
   ui->lUnstagedCount->setText(QString("(%1)").arg(ui->stagedFilesList->count()));
}
