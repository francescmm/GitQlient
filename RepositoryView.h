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
class Rev;

class RepositoryView : public QTreeView
{
   Q_OBJECT
   struct DropInfo;

signals:
   void signalViewUpdated();
   void signalOpenDiff(const QString &sha);
   void signalAmmendCommit(const QString &sha);

public:
   explicit RepositoryView(QWidget *parent = nullptr);
   ~RepositoryView();
   void setup(Domain *d);
   const QString shaFromAnnId(int id);
   void scrollToCurrent(ScrollHint hint = EnsureVisible);
   void scrollToNext(int direction);
   void getSelectedItems(QStringList &selectedItems);
   bool update();
   void addNewRevs(const QVector<QString> &shaVec);
   const QString sha(int row) const;
   int row(const QString &sha) const;
   void markDiffToSha(const QString &sha);

signals:
   void rebase(const QString &from, const QString &to, const QString &onto);
   void merge(const QStringList &shas, const QString &into);
   void moveRef(const QString &refName, const QString &toSHA);
   void contextMenu(const QString &, int);
   void diffTargetChanged(int); // used by new model_view integration

private slots:
   virtual void currentChanged(const QModelIndex &, const QModelIndex &);

private:
   void showContextMenu(const QPoint &);
   void setupGeometry();
   bool filterRightButtonPressed(QMouseEvent *e);
   bool getLaneParentsChildren(const QString &sha, int x, QStringList &p, QStringList &c);
   int getLaneType(const QString &sha, int pos) const;

   Domain *d = nullptr;
   StateInfo *st = nullptr;
   RepositoryModel *fh = nullptr;
   unsigned long secs;
   bool filterNextContextMenuRequest;
};
