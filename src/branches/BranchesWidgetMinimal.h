#pragma once

#include <QFrame>

class RevisionsCache;
class GitBase;
class QPushButton;
class QToolButton;
class QMenu;

class BranchesWidgetMinimal : public QFrame
{
   Q_OBJECT
signals:
   void showFullBranchesView();

public:
   explicit BranchesWidgetMinimal(const QSharedPointer<RevisionsCache> &cache, const QSharedPointer<GitBase> git,
                                  QWidget *parent = nullptr);

   void configure();

private:
   QSharedPointer<GitBase> mGit;
   QSharedPointer<RevisionsCache> mCache;
   QPushButton *mBack = nullptr;
   QToolButton *mLocal = nullptr;
   QToolButton *mRemote = nullptr;
   QToolButton *mTags = nullptr;
   QToolButton *mStashes = nullptr;
   QToolButton *mSubmodules = nullptr;

   bool eventFilter(QObject *obj, QEvent *event);
   void configureLocalMenu();
   void configureRemoteMenu();
   void configureTagsMenu();
   void configureStashesMenu();
   void configureSubmodulesMenu();
};
