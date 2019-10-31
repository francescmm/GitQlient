#pragma once

/*
        Author: Marco Costalba (C) 2005-2007

Copyright: See COPYING file that comes with this distribution
                */

#include <QItemDelegate>
#include <QRegExp>
#include <QTreeView>

class Git;
class StateInfo;
class Domain;
class RepositoryModel;
class Revision;
class RevisionsCache;
enum class RepositoryModelColumns;

class RepositoryView : public QTreeView
{
   Q_OBJECT
   struct DropInfo;

signals:
   void signalViewUpdated();
   void signalOpenDiff(const QString &sha);
   void signalAmendCommit(const QString &sha);

public:
   explicit RepositoryView(QSharedPointer<RevisionsCache> revCache, QSharedPointer<Git> git, QWidget *parent = nullptr);
   ~RepositoryView();
   void setup();
   const QString shaFromAnnId(int id);
   void scrollToCurrent(ScrollHint hint = EnsureVisible);
   void scrollToNext(int direction);
   void getSelectedItems(QStringList &selectedItems);
   bool update();
   void addNewRevs(const QVector<QString> &shaVec);
   const QString sha(int row) const;
   int row(const QString &sha) const;
   void markDiffToSha(const QString &sha);
   void clear(bool complete);
   Domain *domain();
   void focusOnCommit(const QString &goToSha);
   QVariant data(int row, RepositoryModelColumns column) const;
   RepositoryModel *model() const { return mRepositoryModel; }

signals:
   void diffTargetChanged(int); // used by new model_view integration

private slots:
   virtual void currentChanged(const QModelIndex &, const QModelIndex &);

private:
   QSharedPointer<RevisionsCache> mRevCache;
   QSharedPointer<Git> mGit;

   void showContextMenu(const QPoint &);
   void setupGeometry();
   bool filterRightButtonPressed(QMouseEvent *e);
   bool getLaneParentsChildren(const QString &sha, int x, QStringList &p, QStringList &c);
   int getLaneType(const QString &sha, int pos) const;

   RepositoryModel *mRepositoryModel = nullptr;
   Domain *d = nullptr;
   StateInfo *st = nullptr;
   unsigned long secs;
   bool filterNextContextMenuRequest;
};
