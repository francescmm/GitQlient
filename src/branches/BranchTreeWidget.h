#pragma once

#include <QTreeWidget>

class GitBase;
class GitCache;

class BranchTreeWidget : public QTreeWidget
{
   Q_OBJECT

signals:
   void fullReload();
   void logReload();
   void signalSelectCommit(const QString &sha);
   void signalMergeRequired(const QString &currentBranch, const QString &fromBranch);
   void signalPullConflict();
   void signalRefreshPRsCache();
   void mergeSqushRequested(const QString &origin, const QString &destination);

public:
   explicit BranchTreeWidget(QWidget *parent = nullptr);
   explicit BranchTreeWidget(const QSharedPointer<GitCache> &cache,
                             const QSharedPointer<GitBase> &git,
                             QWidget *parent = nullptr);

   void init(const QSharedPointer<GitCache> &cache, const QSharedPointer<GitBase> &git);

   void setLocalRepo(bool isLocal) { mIsLocal = isLocal; }
   bool isLocal() const { return mIsLocal; }

   void reloadCurrentBranchLink();
   void clearSelection();

   int focusOnBranch(const QString &itemText, int startSearchPos = -1);

protected:
   bool eventFilter(QObject *obj, QEvent *event) override;

private:
   bool mIsLocal = false;
   QSharedPointer<GitCache> mCache;
   QSharedPointer<GitBase> mGit;
   QTreeWidgetItem *mFolderToRemove = nullptr;

   QVector<QTreeWidgetItem *> findChildItem(const QString &text) const;
   void discoverBranchesInFolder(QTreeWidgetItem *folder, QStringList &branches);

   void showBranchesContextMenu(const QPoint &pos);
   void showDeleteFolderMenu(QTreeWidgetItem *item, const QPoint &pos);

   void checkoutBranch(QTreeWidgetItem *item);
   void selectCommit(QTreeWidgetItem *item);
   void onSelectionChanged();
   void deleteFolder();
   void onDeleteBranch();
};
