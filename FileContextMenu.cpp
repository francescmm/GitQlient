#include "FileContextMenu.h"

#include <QApplication>
#include <QClipboard>
#include <QSettings>

FileContextMenu::FileContextMenu(const QString &file, QWidget *parent)
   : QMenu(parent)
{
   const auto fileHistoryAction = addAction(tr("File history"));
   fileHistoryAction->setEnabled(false);
   // connect(fileHistoryAction, &QAction::triggered, this, &FileList::executeAction);

   const auto fileBlameAction = addAction(tr("File blame"));
   fileBlameAction->setEnabled(false);
   // connect(fileBlameAction, &QAction::triggered, this, &FileList::executeAction);

   const auto fileDiffAction = addAction(tr("Diff"));
   connect(fileDiffAction, &QAction::triggered, this, &FileContextMenu::signalOpenFileDiff);

   addSeparator();

   const auto showInFolderAction = addAction(tr("Show in folder"));
   showInFolderAction->setEnabled(false);
   // connect(showInFolderAction, &QAction::triggered, this, &FileList::executeAction);

   const auto copyPathAction = addAction(tr("Copy path"));
   connect(copyPathAction, &QAction::triggered, this, [file]() {
      QSettings settings;
      const auto fullPath = QString("%1/%2").arg(settings.value("WorkingDirectory").toString(), file);
      QApplication::clipboard()->setText(fullPath);
   });
}
