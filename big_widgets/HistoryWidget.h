#pragma once

#include <QFrame>

class Git;
class CommitHistoryModel;
class CommitHistoryView;
class QLineEdit;
class BranchesWidget;
class QStackedWidget;
class WorkInProgressWidget;
class CommitInfoWidget;

class HistoryWidget : public QFrame
{
   Q_OBJECT

signals:
   void signalViewUpdated();
   void signalUpdateUi();
   void signalOpenDiff(const QString &sha);
   void signalOpenCompareDiff(const QStringList &sha);
   void signalGoToSha(const QString &sha);
   void signalUpdateCache();
   void signalOpenSubmodule(const QString &submodule);
   void signalShowDiff(const QString &sha, const QString &parentSha, const QString &fileName);
   void signalChangesCommitted(bool commited);
   void signalShowFileHistory(const QString &fileName);
   void signalOpenFileCommit(const QString &currentSha, const QString &previousSha, const QString &file);

public:
   explicit HistoryWidget(const QSharedPointer<Git> git, QWidget *parent = nullptr);
   void clear();
   void reload();
   void updateUiFromWatcher();
   void focusOnCommit(const QString &sha);
   void onCommitSelected(const QString &goToSha);
   void onAmendCommit(const QString &sha);
   QString getCurrentSha() const;

private:
   CommitHistoryModel *mRepositoryModel = nullptr;
   CommitHistoryView *mRepositoryView = nullptr;
   BranchesWidget *mBranchesWidget = nullptr;
   QLineEdit *mGoToSha = nullptr;
   QStackedWidget *commitStackedWidget = nullptr;
   WorkInProgressWidget *mCommitWidget = nullptr;
   CommitInfoWidget *mRevisionWidget = nullptr;

   void goToSha();
   void commitSelected(const QModelIndex &index);
   void openDiff(const QModelIndex &index);
};
