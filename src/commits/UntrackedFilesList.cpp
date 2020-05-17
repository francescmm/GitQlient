#include "UntrackedFilesList.h"

#include <QMenu>
#include <QProcess>

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
   QProcess p;
   p.setWorkingDirectory(mWorkingDir);
   p.start(QString("rm -rf %1").arg(mSelectedItem->toolTip()));

   if (p.waitForFinished())
      emit signalCheckoutPerformed();
}

void UntrackedFilesList::onDoubleClick(QListWidgetItem *item)
{
   emit signalShowDiff(item->toolTip());
}
