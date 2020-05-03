#include "FileContextMenu.h"

#include <GitQlientSettings.h>

#include <QApplication>
#include <QClipboard>

FileContextMenu::FileContextMenu(const QString &file, QWidget *parent)
   : QMenu(parent)
{
   setAttribute(Qt::WA_DeleteOnClose);

   const auto fileHistoryAction = addAction(tr("History"));
   fileHistoryAction->setEnabled(false);

   connect(addAction(tr("Blame")), &QAction::triggered, this, &FileContextMenu::signalShowFileHistory);

   const auto fileDiffAction = addAction(tr("Diff"));
   connect(fileDiffAction, &QAction::triggered, this, &FileContextMenu::signalOpenFileDiff);

   addSeparator();

   GitQlientSettings settings;

   if (!settings.value("isGitQlient", false).toBool())
      connect(addAction("Edit file"), &QAction::triggered, this, [this]() { emit signalEditFile(); });

   addSeparator();

   const auto copyPathAction = addAction(tr("Copy path"));
   connect(copyPathAction, &QAction::triggered, this, [file]() {
      QSettings settings;
      const auto fullPath = QString("%1/%2").arg(settings.value("WorkingDirectory").toString(), file);
      QApplication::clipboard()->setText(fullPath);
   });
}
