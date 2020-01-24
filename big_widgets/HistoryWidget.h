#pragma once

#include <QFrame>

class RevisionsCache;
class GitBase;
class CommitHistoryModel;
class CommitHistoryView;
class QLineEdit;
class BranchesWidget;
class QStackedWidget;
class WorkInProgressWidget;
class CommitInfoWidget;
class QCheckBox;

class HistoryWidget : public QFrame
{
   Q_OBJECT

signals:
   void signalViewUpdated();
   void signalUpdateUi();
   void signalOpenDiff(const QString &sha);
   void signalOpenCompareDiff(const QStringList &sha);
   void signalUpdateCache();
   void signalOpenSubmodule(const QString &submodule);
   void signalShowDiff(const QString &sha, const QString &parentSha, const QString &fileName);
   void signalChangesCommitted(bool commited);
   void signalShowFileHistory(const QString &fileName);
   void signalOpenFileCommit(const QString &currentSha, const QString &previousSha, const QString &file);
   void signalAllBranchesActive(bool showAll);

public:
   explicit HistoryWidget(const QSharedPointer<RevisionsCache> &cache, const QSharedPointer<GitBase> git,
                          QWidget *parent = nullptr);
   void clear();
   void reload();
   void updateUiFromWatcher();
   void focusOnCommit(const QString &sha);
   void onCommitSelected(const QString &goToSha);
   void onAmendCommit(const QString &sha);
   QString getCurrentSha() const;
   void onNewRevisions(int totalCommits);

private:
   QSharedPointer<GitBase> mGit;
   QSharedPointer<RevisionsCache> mCache;
   CommitHistoryModel *mRepositoryModel = nullptr;
   CommitHistoryView *mRepositoryView = nullptr;
   BranchesWidget *mBranchesWidget = nullptr;
   QLineEdit *mSearchInput = nullptr;
   QStackedWidget *mCommitStackedWidget = nullptr;
   WorkInProgressWidget *mCommitWidget = nullptr;
   CommitInfoWidget *mRevisionWidget = nullptr;
   QCheckBox *mChShowAllBranches = nullptr;

   void search();
   void goToSha(const QString &sha);
   void commitSelected(const QModelIndex &index);
   void openDiff(const QModelIndex &index);
   void onShowAllUpdated(bool showAll);
   void onBranchCheckout();
};
