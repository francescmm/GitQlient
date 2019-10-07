/*
        Author: Marco Costalba (C) 2005-2007

Copyright: See COPYING file that comes with this distribution
                */
#include "FileListWidget.h"

#include <FileContextMenu.h>
#include "domain.h"
#include "git.h"

#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QMenu>

FileListWidget::FileListWidget(QWidget *p)
   : QListWidget(p)
   , d(nullptr)
   , st(nullptr)
{
   setContextMenuPolicy(Qt::CustomContextMenu);
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
}

QString FileListWidget::currentText()
{
   const auto item = currentItem();
   return item ? item->data(Qt::DisplayRole).toString() : "";
}

void FileListWidget::showContextMenu(const QPoint &pos)
{
   const auto item = itemAt(pos);

   if (item)
   {
      const auto fileName = item->data(Qt::DisplayRole).toString();
      const auto menu = new FileContextMenu(fileName, this);
      connect(menu, &FileContextMenu::signalOpenFileDiff, this,
              [this, item] { emit QListWidget::itemDoubleClicked(item); });
      menu->exec(viewport()->mapToGlobal(pos));
   }
}

void FileListWidget::insertFiles(const RevFile *files)
{
   clear();

   if (!files)
      return;

   if (st->isMerge())
   {
      const QString header((st->allMergeFiles()) ? QString("Click to view only interesting files")
                                                 : QString("Click to view all merge files"));
      const bool useDark = QPalette().color(QPalette::Window).value() > QPalette().color(QPalette::WindowText).value();
      QColor color(Qt::blue);
      if (!useDark)
         color = color.lighter();
      addItem(header, color);
   }
   if (files->count() == 0)
      return;

   bool isMergeParents = !files->mergeParent.isEmpty();
   int prevPar = (isMergeParents ? files->mergeParent.first() : 1);
   setUpdatesEnabled(false);
   for (auto i = 0; i < files->count(); ++i)
   {

      if (files->statusCmp(i, RevFile::UNKNOWN))
         continue;

      QColor clr = palette().color(QPalette::WindowText);
      if (isMergeParents && files->mergeParent.at(i) != prevPar)
      {
         prevPar = files->mergeParent.at(i);
         new QListWidgetItem("", this);
         new QListWidgetItem("", this);
      }
      QString extSt(files->extendedStatus(i));
      if (extSt.isEmpty())
      {
         if (files->statusCmp(i, RevFile::NEW))
            clr = QColor("#50FA7B");
         else if (files->statusCmp(i, RevFile::DELETED))
            clr = QColor("#FF5555");
         else
            clr = Qt::white;
      }
      else
      {
         clr = QColor("#579BD5");
         // in case of rename deleted file is not shown and...
         if (files->statusCmp(i, RevFile::DELETED))
            continue;

         // ...new file is shown with extended info
         if (files->statusCmp(i, RevFile::NEW))
         {
            addItem(extSt, clr);
            continue;
         }
      }
      addItem(Git::getInstance()->filePath(*files, i), clr);
   }
   setUpdatesEnabled(true);
}

void FileListWidget::update(const RevFile *files, bool newFiles)
{
   QPalette pl = QApplication::palette();

   if (!st->diffToSha().isEmpty())
      pl.setColor(QPalette::Base, QColor("blue"));

   setPalette(pl);

   if (newFiles)
      insertFiles(files);

   QString fileName(currentText());
   Git::getInstance()->removeExtraFileInfo(&fileName); // could be a renamed/copied file

   if (!fileName.isEmpty() && (fileName == st->fileName()))
   {
      currentItem()->setSelected(st->selectItem()); // just a refresh
      return;
   }
   clearSelection();

   if (st->fileName().isEmpty())
      return;

   QList<QListWidgetItem *> l = findItems(st->fileName(), Qt::MatchExactly);
   if (l.isEmpty())
   { // could be a renamed/copied file, try harder
      fileName = st->fileName();
      Git::getInstance()->addExtraFileInfo(&fileName, st->sha(), st->diffToSha(), st->allMergeFiles());
      l = findItems(fileName, Qt::MatchExactly);
   }
   if (!l.isEmpty())
   {
      setCurrentItem(l.first());
      l.first()->setSelected(st->selectItem());
   }
}
