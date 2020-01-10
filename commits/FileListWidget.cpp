#include "FileListWidget.h"

#include <FileContextMenu.h>
#include <RevisionFile.h>
#include <FileListDelegate.h>
#include <GitRepoLoader.h>
#include <GitQlientStyles.h>
#include <RevisionsCache.h>

#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QMenu>
#include <QItemDelegate>

FileListWidget::FileListWidget(const QSharedPointer<GitBase> &git, const QSharedPointer<RevisionsCache> &cache,
                               QWidget *p)
   : QListWidget(p)
   , mGit(git)
   , mCache(cache)
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

void FileListWidget::insertFiles(const QString &currentSha, const QString &compareToSha)
{
   clear();

   RevisionFile files;

   if (mCache->containsRevisionFile(currentSha, compareToSha))
      files = mCache->getRevisionFile(currentSha, compareToSha);
   else if (mCache->getCommitInfo(currentSha).parentsCount() > 0)
   {
      QScopedPointer<GitRepoLoader> git(new GitRepoLoader(mGit, mCache));
      git->updateWipRevision();
      files = mCache->getRevisionFile(CommitInfo::ZERO_SHA, compareToSha);
   }

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

               clr = fileRename.isEmpty() ? GitQlientStyles::getGreen() : GitQlientStyles::getBlue();
               fileName = fileRename.isEmpty() ? files.getFile(i) : fileRename;
            }
            else
            {
               clr = files.statusCmp(i, RevisionFile::DELETED) ? GitQlientStyles::getRed()
                                                               : GitQlientStyles::getTextColor();
               fileName = files.getFile(i);
            }

            addItem(fileName, clr);
         }
      }

      setUpdatesEnabled(true);
   }
}
