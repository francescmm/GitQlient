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
#include <QTabWidget>

FileHistoryWidget::FileHistoryWidget(const QSharedPointer<Git> &git, QWidget *parent)
   : QFrame(parent)
   , mGit(git)
   , fileSystemModel(new QFileSystemModel())
   , mRepoModel(new CommitHistoryModel(mGit))
   , mRepoView(new CommitHistoryView(mGit))
   , fileSystemView(new QTreeView())
   , mTabWidget(new QTabWidget())
{
   mTabWidget->setObjectName("HistoryTab");
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
   connect(mRepoView, &CommitHistoryView::clicked, this,
           qOverload<const QModelIndex &>(&FileHistoryWidget::reloadBlame));

   fileSystemModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);

   // fileSystemView->setItemDelegate(new BranchesViewDelegate());
   fileSystemView->setModel(fileSystemModel);
   fileSystemView->setMaximumWidth(450);
   fileSystemView->header()->setSectionHidden(1, true);
   fileSystemView->header()->setSectionHidden(2, true);
   fileSystemView->header()->setSectionHidden(3, true);
   fileSystemView->setContextMenuPolicy(Qt::CustomContextMenu);
   connect(fileSystemView, &QTreeView::clicked, this,
           qOverload<const QModelIndex &>(&FileHistoryWidget::showFileHistory));

   const auto historyBlameLayout = new QGridLayout(this);
   historyBlameLayout->setContentsMargins(QMargins());
   historyBlameLayout->addWidget(mRepoView, 0, 0);
   historyBlameLayout->addWidget(fileSystemView, 1, 0);
   historyBlameLayout->addWidget(mTabWidget, 0, 1, 2, 1);

   mTabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
   connect(mTabWidget, &QTabWidget::currentChanged, this, &FileHistoryWidget::reloadHistory);

   connect(mTabWidget, &QTabWidget::tabCloseRequested, mTabWidget, [this](int index) {
      auto widget = mTabWidget->widget(index);
      mTabWidget->removeTab(index);
      delete widget;
   });
}

void FileHistoryWidget::init(const QString &workingDirectory)
{
   mWorkingDirectory = workingDirectory;
   fileSystemModel->setRootPath(workingDirectory);
   fileSystemView->setRootIndex(fileSystemModel->index(workingDirectory));
}

void FileHistoryWidget::showFileHistory(const QString &filePath)
{
   if (!mTabsMap.contains(filePath))
   {
      const auto ret = mGit->history(filePath);

      if (ret.success)
      {
         const auto shaHistory = ret.output.toString().split("\n", QString::SkipEmptyParts);
         mRepoView->blockSignals(true);
         mRepoView->filterBySha(shaHistory);
         mRepoView->blockSignals(false);

         const auto previousSha = shaHistory.count() > 1 ? shaHistory.at(1) : QString(tr("No info"));
         const auto fileBlameWidget = new FileBlameWidget(mGit);

         fileBlameWidget->setup(filePath, shaHistory.constFirst(), previousSha);
         connect(fileBlameWidget, &FileBlameWidget::signalCommitSelected, mRepoView, &CommitHistoryView::focusOnCommit);

         const auto index = mTabWidget->addTab(fileBlameWidget, filePath.split("/").last());
         mTabWidget->setTabsClosable(true);
         mTabWidget->blockSignals(true);
         mTabWidget->setCurrentIndex(index);
         mTabWidget->blockSignals(false);

         mTabsMap.insert(filePath, fileBlameWidget);
      }
   }
   else
      mTabWidget->setCurrentWidget(mTabsMap.value(filePath));
}

void FileHistoryWidget::reloadBlame(const QModelIndex &index)
{
   const auto sha
       = mRepoView->model()->index(index.row(), static_cast<int>(CommitHistoryColumns::SHA)).data().toString();
   const auto previousSha
       = mRepoView->model()->index(index.row() + 1, static_cast<int>(CommitHistoryColumns::SHA)).data().toString();
   const auto blameWidget = qobject_cast<FileBlameWidget *>(mTabWidget->currentWidget());
   blameWidget->reload(sha, previousSha);
}

void FileHistoryWidget::reloadHistory(int tabIndex)
{
   const auto blameWidget = qobject_cast<FileBlameWidget *>(mTabWidget->widget(tabIndex));
   const auto sha = blameWidget->getCurrentSha();
   const auto file = blameWidget->getCurrentFile();

   const auto ret = mGit->history(file);

   if (ret.success)
   {
      const auto shaHistory = ret.output.toString().split("\n", QString::SkipEmptyParts);
      mRepoView->blockSignals(true);
      mRepoView->filterBySha(shaHistory);
      mRepoView->blockSignals(false);
   }
}

void FileHistoryWidget::showFileHistory(const QModelIndex &index)
{
   auto item = fileSystemModel->fileInfo(index);

   if (item.isFile())
      showFileHistory(item.filePath());
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
      const auto currentFile = qobject_cast<FileBlameWidget *>(mTabWidget->currentWidget())->getCurrentFile();
      emit showFileDiff(sha, parentSha, currentFile);
   });

   menu->exec(mRepoView->viewport()->mapToGlobal(pos));
}
