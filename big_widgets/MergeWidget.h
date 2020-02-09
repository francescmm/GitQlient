#pragma once

#include <QFrame>
#include <QMap>

class GitBase;
class QVBoxLayout;
class QPushButton;
class QStackedWidget;
class MergeInfoWidget;
class QLineEdit;
class QTextEdit;
class FileDiffWidget;
class RevisionFiles;
class RevisionsCache;

class MergeWidget : public QFrame
{
   Q_OBJECT

signals:
   void signalMergeFinished();

public:
   explicit MergeWidget(const QSharedPointer<RevisionsCache> &gitQlientCache, const QSharedPointer<GitBase> &git,
                        QWidget *parent = nullptr);

   void configure(const RevisionFiles &files);

private:
   QSharedPointer<RevisionsCache> mGitQlientCache;
   QSharedPointer<GitBase> mGit;
   QVBoxLayout *mConflictBtnContainer = nullptr;
   QVBoxLayout *mAutoMergedBtnContainer = nullptr;
   QStackedWidget *mCenterStackedWidget = nullptr;
   QLineEdit *mCommitTitle = nullptr;
   QTextEdit *mDescription = nullptr;
   QPushButton *mMergeBtn = nullptr;
   QPushButton *mAbortBtn = nullptr;
   QMap<QPushButton *, FileDiffWidget *> mConflictButtons;

   void fillButtonFileList(const RevisionFiles &files);
   void changeDiffView(bool fileBtnChecked);
   void abort();
   void commit();
   void removeMergeComponents();
};
