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
class ShaFilterProxyModel;
enum class RepositoryModelColumns;

class RepositoryView : public QTreeView
{
   Q_OBJECT
   struct DropInfo;

signals:
   void signalViewUpdated();
   void signalOpenDiff(const QString &sha);
   void signalOpenCompareDiff(const QStringList &sha);
   void signalAmendCommit(const QString &sha);

public:
   explicit RepositoryView(QSharedPointer<RevisionsCache> revCache, QSharedPointer<Git> git, QWidget *parent = nullptr);
   ~RepositoryView();
   void setup();
   QList<QString> getSelectedSha() const;
   void filterBySha(const QStringList &shaList);
   bool hasActiveFiler() const { return mIsFiltering; }

   bool update();
   void clear(bool complete);
   Domain *domain();
   void focusOnCommit(const QString &goToSha);
   QVariant data(int row, RepositoryModelColumns column) const;

signals:
   void diffTargetChanged(int); // used by new model_view integration

private slots:
   virtual void currentChanged(const QModelIndex &, const QModelIndex &);

private:
   QSharedPointer<RevisionsCache> mRevCache;
   QSharedPointer<Git> mGit;
   RepositoryModel *mRepositoryModel = nullptr;
   Domain *d = nullptr;
   StateInfo *st = nullptr;
   ShaFilterProxyModel *mProxyModel = nullptr;
   bool mIsFiltering = false;

   void showContextMenu(const QPoint &);
   void saveHeaderState();

   void setupGeometry();
   bool filterRightButtonPressed(QMouseEvent *e);
   bool getLaneParentsChildren(const QString &sha, int x, QStringList &p, QStringList &c);
   int getLaneType(const QString &sha, int pos) const;

   unsigned long secs;
};
