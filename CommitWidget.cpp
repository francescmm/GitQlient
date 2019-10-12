/*
        Description: changes commit dialog

Author: Marco Costalba (C) 2005-2007

             Copyright: See COPYING file that comes with this distribution
                             */
#include <CommitWidget.h>
#include <ui_CommitWidget.h>

#include <git.h>

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

using namespace QGit;

QString CommitWidget::lastMsgBeforeError;

CommitWidget::CommitWidget(QSharedPointer<Git> git, QWidget *parent)
   : QWidget(parent)
   , ui(new Ui::CommitWidget)
   , mGit(git)
{
   ui->setupUi(this);

   QIcon stagedIcon(":/icons/staged");
   ui->stagedFilesIcon->setPixmap(stagedIcon.pixmap(15, 15));

   QIcon unstagedIcon(":/icons/unstaged");
   ui->unstagedIcon->setPixmap(unstagedIcon.pixmap(15, 15));

   connect(ui->leCommitTitle, &QLineEdit::returnPressed, this, &CommitWidget::applyChanges);
   connect(ui->pbCommit, &QPushButton::clicked, this, &CommitWidget::applyChanges);
   connect(ui->listWidgetFiles, &QListWidget::customContextMenuRequested, this, &CommitWidget::contextMenuPopup);
   connect(ui->listWidgetFiles, &QListWidget::itemClicked, this, &CommitWidget::addFileToCommitList);
   connect(ui->filesWidget, &QListWidget::itemClicked, this, &CommitWidget::removeFileFromCommitList);
}

void CommitWidget::init(const QString &shaToAmmend)
{
   mIsAmmend = shaToAmmend != ZERO_SHA;

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

   const auto files = mGit->getFiles(mIsAmmend ? shaToAmmend : ZERO_SHA);

   if (files)
   {
      for (auto i = 0; i < files->count(); ++i)
      {
         QColor myColor;

         if (files->statusCmp(i, RevFile::NEW) || files->statusCmp(i, RevFile::UNKNOWN))
            myColor = Qt::darkGreen;
         else if (files->statusCmp(i, RevFile::DELETED))
            myColor = Qt::red;
         else
            myColor = Qt::white;

         const auto item = new QListWidgetItem(ui->listWidgetFiles);
         item->setText(mGit->filePath(*files, i));
         item->setForeground(myColor);
      }
   }

   ui->lUnstagedCount->setText(QString("(%1)").arg(ui->listWidgetFiles->count()));
   ui->lStagedCount->setText(QString("(%1)").arg(ui->filesWidget->count()));

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
   ui->pbCommit->setEnabled(ui->filesWidget->count());
}

void CommitWidget::addFileToCommitList(QListWidgetItem *item)
{
   const auto row = ui->listWidgetFiles->row(item);
   ui->listWidgetFiles->takeItem(row);
   ui->filesWidget->addItem(item);
   ui->lUnstagedCount->setText(QString("(%1)").arg(ui->listWidgetFiles->count()));
   ui->lStagedCount->setText(QString("(%1)").arg(ui->filesWidget->count()));
   ui->pbCommit->setEnabled(true);
}

void CommitWidget::removeFileFromCommitList(QListWidgetItem *item)
{
   const auto row = ui->filesWidget->row(item);
   ui->filesWidget->takeItem(row);
   ui->listWidgetFiles->addItem(item);
   ui->lUnstagedCount->setText(QString("(%1)").arg(ui->listWidgetFiles->count()));
   ui->lStagedCount->setText(QString("(%1)").arg(ui->filesWidget->count()));
   ui->pbCommit->setDisabled(ui->filesWidget->count() == 0);
}

void CommitWidget::contextMenuPopup(const QPoint &pos)
{
   const auto fileName = ui->listWidgetFiles->itemAt(pos)->data(Qt::DisplayRole).toString();
   const auto contextMenu = new QMenu(this);

   connect(contextMenu->addAction("Checkout file"), &QAction::triggered, this, [this, fileName]() {
      const auto ret = mGit->resetFile(fileName);

      if (ret)
         emit signalChangesCommitted(ret);
   });

   connect(contextMenu->addAction("Add file to commit"), &QAction::triggered, this, [this, fileName]() {
      const auto ret = mGit->resetFile(fileName);

      if (ret)
         emit signalChangesCommitted(ret);
   });

   contextMenu->popup(mapToGlobal(pos));
}

void CommitWidget::applyChanges()
{
   mIsAmmend ? ammendChanges() : commitChanges();
}

QStringList CommitWidget::getFiles()
{
   QStringList selFiles;
   const auto totalItems = ui->filesWidget->count();

   for (auto i = 0; i < totalItems; ++i)
      selFiles.append(ui->filesWidget->item(i)->text());

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
   ui->listWidgetFiles->clear();
   ui->leCommitTitle->clear();
   ui->teDescription->clear();
   ui->filesWidget->clear();
   ui->pbCommit->setEnabled(false);
   ui->lUnstagedCount->setText(QString("(%1)").arg(ui->listWidgetFiles->count()));
   ui->lUnstagedCount->setText(QString("(%1)").arg(ui->filesWidget->count()));
}
