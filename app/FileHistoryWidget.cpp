#include "FileHistoryWidget.h"

#include <git.h>
#include <FileBlameWidget.h>
#include <BranchesViewDelegate.h>

#include <QFileSystemModel>
#include <QTreeView>
#include <QGridLayout>
#include <RepositoryView.h>
#include <QHeaderView>
#include <QMenu>

FileHistoryWidget::FileHistoryWidget(QSharedPointer<RevisionsCache> revCache, QSharedPointer<Git> git, QWidget *parent)
   : QFrame(parent)
   , mGit(git)
   , fileSystemModel(new QFileSystemModel())
   , fileSystemView(new QTreeView())
   , mFileBlameWidget(new FileBlameWidget(mGit))
{
   fileSystemModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);

   const auto repoView = new RepositoryView(revCache, mGit);
   repoView->setup();
   repoView->setEnabled(true);
   repoView->setMaximumWidth(400);

   // fileSystemView->setItemDelegate(new BranchesViewDelegate());
   fileSystemView->setModel(fileSystemModel);
   fileSystemView->setMaximumWidth(400);
   fileSystemView->header()->setSectionHidden(1, true);
   fileSystemView->header()->setSectionHidden(2, true);
   fileSystemView->header()->setSectionHidden(3, true);
   fileSystemView->setContextMenuPolicy(Qt::CustomContextMenu);
   connect(fileSystemView, &QTreeView::customContextMenuRequested, this, &FileHistoryWidget::showFileSystemContextMenu);

   const auto historyBlameLayout = new QGridLayout(this);
   historyBlameLayout->setContentsMargins(QMargins());
   historyBlameLayout->addWidget(repoView, 0, 0);
   historyBlameLayout->addWidget(fileSystemView, 1, 0);
   historyBlameLayout->addWidget(mFileBlameWidget, 0, 1, 2, 1);

   connect(mFileBlameWidget, &FileBlameWidget::signalCommitSelected, repoView, &RepositoryView::focusOnCommit);
}

void FileHistoryWidget::init(const QString &workingDirectory)
{
   mWorkingDirectory = workingDirectory;
   fileSystemModel->setRootPath(workingDirectory);
   fileSystemView->setRootIndex(fileSystemModel->index(workingDirectory));
}

void FileHistoryWidget::showFileHistory(const QString &file)
{
   mFileBlameWidget->setup(file);
}

void FileHistoryWidget::showFileSystemContextMenu(const QPoint &pos)
{
   const auto index = fileSystemView->currentIndex();
   auto item = fileSystemModel->filePath(index);
   item.remove(mGit->getWorkingDir().append("/"));

   if (!item.isEmpty())
   {
      const auto menu = new QMenu(this);
      const auto blameAction = menu->addAction(tr("Blame"));
      connect(blameAction, &QAction::triggered, this,
              [this, item]() { showFileHistory(QString("%1/%2").arg(mWorkingDirectory, item)); });

      menu->exec(fileSystemView->mapToGlobal(pos));
   }
}
