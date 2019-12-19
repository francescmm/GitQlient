#include "FileHistoryWidget.h"

#include <git.h>
#include <FileBlameWidget.h>
#include <BranchesViewDelegate.h>
#include <RepositoryViewDelegate.h>
#include <CommitHistoryModel.h>
#include <CommitHistoryView.h>
#include <CommitHistoryColumns.h>
#include <CommitInfo.h>

#include <QFileSystemModel>
#include <QTreeView>
#include <QGridLayout>
#include <QHeaderView>
#include <QMenu>
#include <QApplication>
#include <QClipboard>

FileHistoryWidget::FileHistoryWidget(const QSharedPointer<Git> &git, QWidget *parent)
   : QFrame(parent)
   , mGit(git)
   , fileSystemModel(new QFileSystemModel())
   , mRepoModel(new CommitHistoryModel(mGit))
   , mRepoView(new CommitHistoryView(mGit))
   , fileSystemView(new QTreeView())
   , mFileBlameWidget(new FileBlameWidget(mGit))
{
   mRepoView->setObjectName("blameRepoView");
   mRepoView->setModel(mRepoModel);
   mRepoView->header()->setSectionHidden(static_cast<int>(CommitHistoryColumns::GRAPH), true);
   mRepoView->header()->setSectionHidden(static_cast<int>(CommitHistoryColumns::DATE), true);
   mRepoView->header()->setSectionHidden(static_cast<int>(CommitHistoryColumns::AUTHOR), true);
   mRepoView->setItemDelegate(new RepositoryViewDelegate(mGit, mRepoView));
   mRepoView->setEnabled(true);
   mRepoView->setMaximumWidth(450);
   mRepoView->setSelectionBehavior(QAbstractItemView::SelectRows);
   mRepoView->setSelectionMode(QAbstractItemView::SingleSelection);
   mRepoView->setContextMenuPolicy(Qt::CustomContextMenu);
   mRepoView->activateFilter(true);
   connect(mRepoView, &CommitHistoryView::customContextMenuRequested, this, &FileHistoryWidget::showRepoViewMenu);

   fileSystemModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);

   // fileSystemView->setItemDelegate(new BranchesViewDelegate());
   fileSystemView->setModel(fileSystemModel);
   fileSystemView->setMaximumWidth(450);
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
   mCurrentFile = file;
   const auto ret = mGit->history(mCurrentFile);

   if (ret.success)
      mRepoView->filterBySha(ret.output.toString().split("\n", QString::SkipEmptyParts));

   mFileBlameWidget->setup(mCurrentFile);
}

void FileHistoryWidget::showRepoViewMenu(const QPoint &pos)
{
   const auto sha = mRepoView->getCurrentSha();
   const auto menu = new QMenu(this);
   const auto copyShaAction = menu->addAction(tr("Copy SHA"));
   connect(copyShaAction, &QAction::triggered, this, [sha]() { QApplication::clipboard()->setText(sha); });

   const auto fileDiff = menu->addAction(tr("Show file diff"));
   connect(fileDiff, &QAction::triggered, this, [this, sha]() {
      const auto parentSha = mGit->getCommitInfo(sha).parent(0);
      emit showFileDiff(sha, parentSha, mCurrentFile);
   });

   menu->exec(mRepoView->viewport()->mapToGlobal(pos));
}
