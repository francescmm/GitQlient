/*
        Author: Marco Costalba (C) 2005-2007

Copyright: See COPYING file that comes with this distribution
                */
#include "FileListWidget.h"

#include <FileContextMenu.h>
#include <RevisionFile.h>
#include <FileListDelegate.h>
#include "domain.h"
#include "git.h"

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
}

void FileListWidget::setup(Domain *dm)
{
   d = dm;
   st = &(d->st);

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

void FileListWidget::insertFiles(const RevisionFile *files)
{
   clear();

   if (!files)
      return;

   if (st->isMerge() && !st->allMergeFiles())
      st->setAllMergeFiles(!st->allMergeFiles());

   if (files->count() == 0)
      return;

   bool isMergeParents = !files->mergeParent.isEmpty();
   int prevPar = (isMergeParents ? files->mergeParent.first() : 1);
   setUpdatesEnabled(false);
   for (auto i = 0; i < files->count(); ++i)
   {

      if (files->statusCmp(i, RevisionFile::UNKNOWN))
         continue;

      QColor clr = palette().color(QPalette::WindowText);
      if (isMergeParents && files->mergeParent.at(i) != prevPar)
      {
         prevPar = files->mergeParent.at(i);
         new QListWidgetItem("", this); // WTF?
         new QListWidgetItem("", this); // WTF?
      }
      QString extSt(files->extendedStatus(i));
      if (extSt.isEmpty())
      {
         if (files->statusCmp(i, RevisionFile::NEW))
            clr = QColor("#8DC944");
         else if (files->statusCmp(i, RevisionFile::DELETED))
            clr = QColor("#FF5555");
         else
            clr = Qt::white;
      }
      else
      {
         clr = QColor("#579BD5");
         // in case of rename deleted file is not shown and...
         if (files->statusCmp(i, RevisionFile::DELETED))
            continue;

         // ...new file is shown with extended info
         if (files->statusCmp(i, RevisionFile::NEW))
         {
            addItem(extSt, clr);
            continue;
         }
      }
      addItem(mGit->filePath(*files, i), clr);
   }
   setUpdatesEnabled(true);
}

void FileListWidget::update(const RevisionFile *files, bool newFiles)
{
   QPalette pl = QApplication::palette();

   if (!st->diffToSha().isEmpty())
      pl.setColor(QPalette::Base, QColor("blue"));

   setPalette(pl);

   if (newFiles)
      insertFiles(files);

   const auto item = currentItem();
   const auto currentText = item ? item->data(Qt::DisplayRole).toString() : "";
   QString fileName(currentText);
   mGit->removeExtraFileInfo(&fileName); // could be a renamed/copied file

   if (item && !fileName.isEmpty() && (fileName == st->fileName()))
   {
      item->setSelected(st->selectItem()); // just a refresh
      return;
   }

   clearSelection();

   if (st->fileName().isEmpty())
      return;

   auto l = findItems(st->fileName(), Qt::MatchExactly);

   if (l.isEmpty())
   { // could be a renamed/copied file, try harder
      fileName = st->fileName();
      mGit->addExtraFileInfo(&fileName, st->sha(), st->diffToSha(), st->allMergeFiles());
      l = findItems(fileName, Qt::MatchExactly);
   }

   if (!l.isEmpty())
   {
      setCurrentItem(l.first());
      l.first()->setSelected(st->selectItem());
   }
}
