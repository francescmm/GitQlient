#include "FileHistoryWidget.h"

#include <git.h>
#include <FileBlameWidget.h>
#include <BranchesViewDelegate.h>

#include <QFileSystemModel>
#include <QTreeView>
#include <QGridLayout>
#include <CommitHistoryView.h>
#include <QHeaderView>
#include <QMenu>

FileHistoryWidget::FileHistoryWidget(QSharedPointer<RevisionsCache> revCache, QSharedPointer<Git> git, QWidget *parent)
   : QFrame(parent)
   , mGit(git)
   , fileSystemModel(new QFileSystemModel())
   , mRepoView(new CommitHistoryView(revCache, mGit))
   , fileSystemView(new QTreeView())
   , mFileBlameWidget(new FileBlameWidget(mGit))
{
   fileSystemModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);

   mRepoView->setObjectName("blameRepoView");
   mRepoView->setEnabled(true);
   mRepoView->setMaximumWidth(400);

   // fileSystemView->setItemDelegate(new BranchesViewDelegate());
   fileSystemView->setModel(fileSystemModel);
   fileSystemView->setMaximumWidth(400);
   fileSystemView->header()->setSectionHidden(1, true);
   fileSystemView->header()->setSectionHidden(2, true);
   fileSystemView->header()->setSectionHidden(3, true);
   fileSystemView->setContextMenuPolicy(Qt::CustomContextMenu);
   connect(fileSystemView, &QTreeView::clicked, this, [this](const QModelIndex &index) {
      auto item = fileSystemModel->fileInfo(index);

      if (item.isFile())
         showFileHistory(QString("%1").arg(item.filePath()));
   });

   const auto historyBlameLayout = new QGridLayout(this);
   historyBlameLayout->setContentsMargins(QMargins());
   historyBlameLayout->addWidget(mRepoView, 0, 0);
   historyBlameLayout->addWidget(fileSystemView, 1, 0);
   historyBlameLayout->addWidget(mFileBlameWidget, 0, 1, 2, 1);

   connect(mFileBlameWidget, &FileBlameWidget::signalCommitSelected, mRepoView, &CommitHistoryView::focusOnCommit);
}

void FileHistoryWidget::init(const QString &workingDirectory)
{
   mWorkingDirectory = workingDirectory;
   fileSystemModel->setRootPath(workingDirectory);
   fileSystemView->setRootIndex(fileSystemModel->index(workingDirectory));
}

void FileHistoryWidget::showFileHistory(const QString &file)
{
   const auto ret = mGit->history(file);

   if (ret.success)
      mRepoView->filterBySha(ret.output.toString().split("\n", QString::SkipEmptyParts));

   mFileBlameWidget->setup(file);
}
