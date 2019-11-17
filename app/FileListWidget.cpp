#include "FileListWidget.h"

#include <FileContextMenu.h>
#include <RevisionFile.h>
#include <FileListDelegate.h>
#include <git.h>

#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QMenu>
#include <QItemDelegate>

FileListWidget::FileListWidget(QSharedPointer<Git> git, QWidget *p)
   : QListWidget(p)
   , mGit(git)
{
   setContextMenuPolicy(Qt::CustomContextMenu);
   setItemDelegate(new FileListDelegate(this));

   connect(this, &FileListWidget::customContextMenuRequested, this, &FileListWidget::showContextMenu);
}

void FileListWidget::addItem(const QString &label, const QColor &clr)
{
   const auto item = new QListWidgetItem(label, this);
   item->setForeground(clr);
   item->setToolTip(label);
}

void FileListWidget::showContextMenu(const QPoint &pos)
{
   const auto item = itemAt(pos);

   if (item)
   {
      const auto fileName = item->data(Qt::DisplayRole).toString();
      const auto menu = new FileContextMenu(fileName, this);
      connect(menu, &FileContextMenu::signalShowFileHistory, this,
              [this, fileName]() { emit signalShowFileHistory(fileName); });
      connect(menu, &FileContextMenu::signalOpenFileDiff, this,
              [this, item] { emit QListWidget::itemDoubleClicked(item); });
      menu->exec(viewport()->mapToGlobal(pos));
   }
}

void FileListWidget::insertFiles(const QString currentSha, const QString &compareToSha)
{
   clear();

   const auto files = mGit->getDiffFiles(currentSha, compareToSha, true);

   if (files.count() != 0)
   {
      setUpdatesEnabled(false);

      for (auto i = 0; i < files.count(); ++i)
      {
         if (!files.statusCmp(i, RevisionFile::UNKNOWN))
         {
            QColor clr;
            QString fileName;

            if (files.statusCmp(i, RevisionFile::NEW))
            {
               const auto fileRename = files.extendedStatus(i);

               clr = fileRename.isEmpty() ? QColor("#8DC944") : QColor("#579BD5");
               fileName = fileRename.isEmpty() ? mGit->filePath(files, i) : fileRename;
            }
            else
            {
               clr = files.statusCmp(i, RevisionFile::DELETED) ? QColor("#FF5555") : Qt::white;
               fileName = mGit->filePath(files, i);
            }

            addItem(fileName, clr);
         }
      }

      setUpdatesEnabled(true);
   }
}
