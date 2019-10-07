#include "FileContextMenu.h"

#include <QApplication>
#include <QClipboard>
#include <QSettings>

FileContextMenu::FileContextMenu(const QString &file, QWidget *parent)
   : QMenu(parent)
{
   const auto fileHistoryAction = new QAction("File history");
   fileHistoryAction->setEnabled(false);
   // connect(fileHistoryAction, &QAction::triggered, this, &FileList::executeAction);
   addAction(fileHistoryAction);

   const auto fileBlameAction = new QAction("File blame");
   fileBlameAction->setEnabled(false);
   // connect(fileBlameAction, &QAction::triggered, this, &FileList::executeAction);
   addAction(fileBlameAction);

   const auto fileDiffAction = new QAction("Diff");
   connect(fileDiffAction, &QAction::triggered, this, &FileContextMenu::signalOpenFileDiff);
   addAction(fileDiffAction);

   addSeparator();

   const auto showInFolderAction = new QAction("Show in folder");
   showInFolderAction->setEnabled(false);
   // connect(showInFolderAction, &QAction::triggered, this, &FileList::executeAction);
   addAction(showInFolderAction);

   const auto copyPathAction = new QAction("Copy path");
   connect(copyPathAction, &QAction::triggered, this, [file]() {
      QSettings settings;
      const auto fullPath = QString("%1/%2").arg(settings.value("WorkingDirectory").toString(), file);
      QApplication::clipboard()->setText(fullPath);
   });
   addAction(copyPathAction);
}
