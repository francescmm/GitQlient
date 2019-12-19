#include "FileContextMenu.h"

#include <QApplication>
#include <QClipboard>
#include <QSettings>

FileContextMenu::FileContextMenu(const QString &file, QWidget *parent)
   : QMenu(parent)
{
   const auto fileHistoryAction = addAction(tr("History"));
   fileHistoryAction->setEnabled(false);
   // connect(fileHistoryAction, &QAction::triggered, this, &FileList::executeAction);

   connect(addAction(tr("Blame")), &QAction::triggered, this, &FileContextMenu::signalShowFileHistory);

   const auto fileDiffAction = addAction(tr("Diff"));
   connect(fileDiffAction, &QAction::triggered, this, &FileContextMenu::signalOpenFileDiff);

   addSeparator();

   const auto copyPathAction = addAction(tr("Copy path"));
   connect(copyPathAction, &QAction::triggered, this, [file]() {
      QSettings settings;
      const auto fullPath = QString("%1/%2").arg(settings.value("WorkingDirectory").toString(), file);
      QApplication::clipboard()->setText(fullPath);
   });
}
