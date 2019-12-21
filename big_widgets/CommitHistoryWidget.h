#ifndef COMMITHISTORYWIDGET_H
#define COMMITHISTORYWIDGET_H

#include <QFrame>

class Git;
class CommitHistoryModel;
class CommitHistoryView;
class QLineEdit;
class BranchesWidget;

class CommitHistoryWidget : public QFrame
{
   Q_OBJECT

signals:
   void signalViewUpdated();
   void signalOpenDiff(const QString &sha);
   void signalOpenCompareDiff(const QStringList &sha);
   void signalAmendCommit(const QString &sha);
   void signalGoToSha(const QString &sha);
   void signalUpdateCache();
   void signalOpenSubmodule(const QString &submodule);

public:
   explicit CommitHistoryWidget(const QSharedPointer<Git> git, QWidget *parent = nullptr);
   void clear();
   void reload();
   void focusOnCommit(const QString &sha);
   QString getCurrentSha() const;

private:
   CommitHistoryModel *mRepositoryModel = nullptr;
   CommitHistoryView *mRepositoryView = nullptr;
   BranchesWidget *mBranchesWidget = nullptr;
   QLineEdit *mGoToSha = nullptr;

   void goToSha();
   void commitSelected(const QModelIndex &index);
   void openDiff(const QModelIndex &index);
};

#endif // COMMITHISTORYWIDGET_H
