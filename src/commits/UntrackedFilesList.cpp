#include "UntrackedFilesList.h"

#include <QMenu>
#include <QProcess>

#include <QLogger.h>

using namespace QLogger;

UntrackedFilesList::UntrackedFilesList(QWidget *parent)
   : QListWidget(parent)
{
   connect(this, &QListWidget::customContextMenuRequested, this, &UntrackedFilesList::onContextMenu);
   connect(this, &QListWidget::itemDoubleClicked, this, &UntrackedFilesList::onDoubleClick);
}

void UntrackedFilesList::onContextMenu(const QPoint &pos)
{
   if (mSelectedItem = itemAt(pos); mSelectedItem != nullptr)
   {
      const auto contextMenu = new QMenu(this);
      connect(contextMenu->addAction(tr("Stage file")), &QAction::triggered, this, &UntrackedFilesList::onStageFile);
      connect(contextMenu->addAction(tr("Delete file")), &QAction::triggered, this, &UntrackedFilesList::onDeleteFile);

      contextMenu->popup(mapToGlobal(mapToParent(pos)));
   }
}

void UntrackedFilesList::onStageFile()
{
   emit signalStageFile(mSelectedItem);
}

void UntrackedFilesList::onDeleteFile()
{
   const auto path = QString("rm -rf %1").arg(mSelectedItem->toolTip());

   QLog_Info("UI", "Removing paht: " + path);

   QProcess p;
   p.setWorkingDirectory(mWorkingDir);
   p.start(path);

   if (p.waitForFinished())
      emit signalCheckoutPerformed();
}

void UntrackedFilesList::onDoubleClick(QListWidgetItem *item)
{
   emit signalShowDiff(item->toolTip());
}
