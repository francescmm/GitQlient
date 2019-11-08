#pragma once

/*
        Description: interface to git programs

        Author: Marco Costalba (C) 2005-2007

        Copyright: See COPYING file that comes with this distribution

*/

#include <QAbstractItemModel>
#include <QSharedPointer>

class RevisionsCache;
class Git;
class Lanes;
class Revision;
enum class RepositoryModelColumns;

class RepositoryModel : public QAbstractItemModel
{
   Q_OBJECT
public:
   explicit RepositoryModel(QSharedPointer<RevisionsCache> revCache, QSharedPointer<Git> git,
                            QObject *parent = nullptr);
   ~RepositoryModel();
   void clear(bool complete = true);
   QString sha(int row) const;
   void setEarlyOutputState(bool b = true) { earlyOutputCnt = (b ? earlyOutputCntBase : -1); }

   QVariant data(const QModelIndex &index, int role) const override;
   QVariant headerData(int s, Qt::Orientation o, int role = Qt::DisplayRole) const override;
   QModelIndex index(int r, int c, const QModelIndex &par = QModelIndex()) const override;
   QModelIndex parent(const QModelIndex &index) const override;
   int rowCount(const QModelIndex &par = QModelIndex()) const override;
   bool hasChildren(const QModelIndex &par = QModelIndex()) const override;
   int columnCount(const QModelIndex &) const override { return mColumns.count(); }

private slots:
   void on_newRevsAdded();
   void on_loadCompleted();

private:
   QSharedPointer<RevisionsCache> mRevCache;
   QSharedPointer<Git> mGit;
   friend class Git;

   QVariant getToolTipData(const Revision &r) const;
   QVariant getDisplayData(const Revision &rev, int column) const;
   void flushTail();

   QMap<RepositoryModelColumns, QString> mColumns;
   int earlyOutputCnt;
   int earlyOutputCntBase;
   int rowCnt = 0;
   QStringList curFNames;
   QStringList renamedRevs;
   QHash<QString, QString> renamedPatches;
};
