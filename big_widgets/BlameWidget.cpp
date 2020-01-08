#include "BlameWidget.h"

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

BlameWidget::BlameWidget(const QSharedPointer<RevisionsCache> &cache, const QSharedPointer<Git> &git, QWidget *parent)
   : QFrame(parent)
   , mGit(git)
   , fileSystemModel(new QFileSystemModel())
   , mRepoModel(new CommitHistoryModel(cache, mGit))
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
   mRepoView->setItemDelegate(new RepositoryViewDelegate(cache, mGit, mRepoView));
   mRepoView->setEnabled(true);
   mRepoView->setMaximumWidth(450);
   mRepoView->setSelectionBehavior(QAbstractItemView::SelectRows);
   mRepoView->setSelectionMode(QAbstractItemView::SingleSelection);
   mRepoView->setContextMenuPolicy(Qt::CustomContextMenu);
   mRepoView->activateFilter(true);
   mRepoView->filterBySha({});
   connect(mRepoView, &CommitHistoryView::customContextMenuRequested, this, &BlameWidget::showRepoViewMenu);
   connect(mRepoView, &CommitHistoryView::clicked, this, qOverload<const QModelIndex &>(&BlameWidget::reloadBlame));

   fileSystemModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);

   // fileSystemView->setItemDelegate(new BranchesViewDelegate());
   fileSystemView->setModel(fileSystemModel);
   fileSystemView->setMaximumWidth(450);
   fileSystemView->header()->setSectionHidden(1, true);
   fileSystemView->header()->setSectionHidden(2, true);
   fileSystemView->header()->setSectionHidden(3, true);
   fileSystemView->setContextMenuPolicy(Qt::CustomContextMenu);
   connect(fileSystemView, &QTreeView::clicked, this, qOverload<const QModelIndex &>(&BlameWidget::showFileHistory));

   const auto historyBlameLayout = new QGridLayout(this);
   historyBlameLayout->setContentsMargins(QMargins());
   historyBlameLayout->addWidget(mRepoView, 0, 0);
   historyBlameLayout->addWidget(fileSystemView, 1, 0);
   historyBlameLayout->addWidget(mTabWidget, 0, 1, 2, 1);

   mTabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
   connect(mTabWidget, &QTabWidget::currentChanged, this, &BlameWidget::reloadHistory);

   connect(mTabWidget, &QTabWidget::tabCloseRequested, mTabWidget, [this](int index) {
      auto widget = mTabWidget->widget(index);
      mTabWidget->removeTab(index);
      delete widget;
   });
}

void BlameWidget::init(const QString &workingDirectory)
{
   mWorkingDirectory = workingDirectory;
   fileSystemModel->setRootPath(workingDirectory);
   fileSystemView->setRootIndex(fileSystemModel->index(workingDirectory));
}

void BlameWidget::showFileHistory(const QString &filePath)
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

void BlameWidget::reloadBlame(const QModelIndex &index)
{
   mSelectedRow = index.row();
   const auto blameWidget = qobject_cast<FileBlameWidget *>(mTabWidget->currentWidget());

   if (blameWidget)
   {
      const auto sha
          = mRepoView->model()->index(index.row(), static_cast<int>(CommitHistoryColumns::SHA)).data().toString();
      const auto previousSha
          = mRepoView->model()->index(index.row() + 1, static_cast<int>(CommitHistoryColumns::SHA)).data().toString();
      blameWidget->reload(sha, previousSha);
   }
}

void BlameWidget::reloadHistory(int tabIndex)
{
   if (tabIndex >= 0)
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

         const auto repoModel = mRepoView->model();
         const auto totalRows = repoModel->rowCount();
         for (auto i = 0; i < totalRows; ++i)
         {
            const auto index = mRepoView->model()->index(i, static_cast<int>(CommitHistoryColumns::SHA));

            if (index.data().toString().startsWith(sha))
            {
               mRepoView->setCurrentIndex(index);
               mRepoView->selectionModel()->select(index,
                                                   QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            }
         }

         mRepoView->blockSignals(false);
      }
   }
}

void BlameWidget::showFileHistory(const QModelIndex &index)
{
   auto item = fileSystemModel->fileInfo(index);

   if (item.isFile())
      showFileHistory(item.filePath());
}

void BlameWidget::showRepoViewMenu(const QPoint &pos)
{
   const auto shaColumnIndex = static_cast<int>(CommitHistoryColumns::SHA);
   const auto sha = mRepoView->model()->index(mSelectedRow, shaColumnIndex).data().toString();
   const auto previousSha = mRepoView->model()->index(mSelectedRow + 1, shaColumnIndex).data().toString();
   const auto menu = new QMenu(this);
   const auto copyShaAction = menu->addAction(tr("Copy SHA"));
   connect(copyShaAction, &QAction::triggered, this, [sha]() { QApplication::clipboard()->setText(sha); });

   const auto fileDiff = menu->addAction(tr("Show file diff"));
   connect(fileDiff, &QAction::triggered, this, [this, sha, previousSha]() {
      const auto currentFile = qobject_cast<FileBlameWidget *>(mTabWidget->currentWidget())->getCurrentFile();
      emit showFileDiff(sha, previousSha, currentFile);
   });

   menu->exec(mRepoView->viewport()->mapToGlobal(pos));
}
