#pragma once

/*
        Author: Marco Costalba (C) 2005-2007

Copyright: See COPYING file that comes with this distribution
                */

#include <QItemDelegate>
#include <QRegExp>
#include <QTreeView>

class Git;
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
   QList<QString> getSelectedShaList() const;
   void filterBySha(const QStringList &shaList);
   bool hasActiveFiler() const { return mIsFiltering; }

   bool update();
   void clear();
   void focusOnCommit(const QString &goToSha);
   QString getCurrentSha() const { return mCurrentSha; }

signals:
   void diffTargetChanged(int); // used by new model_view integration

private slots:
   virtual void currentChanged(const QModelIndex &, const QModelIndex &);

private:
   QSharedPointer<RevisionsCache> mRevCache;
   QSharedPointer<Git> mGit;
   RepositoryModel *mRepositoryModel = nullptr;
   ShaFilterProxyModel *mProxyModel = nullptr;
   bool mIsFiltering = false;
   QString mCurrentSha;

   void showContextMenu(const QPoint &);
   void saveHeaderState();
   void setupGeometry();
};
