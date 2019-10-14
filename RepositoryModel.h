#pragma once

/*
        Description: interface to git programs

        Author: Marco Costalba (C) 2005-2007

        Copyright: See COPYING file that comes with this distribution

*/

#include <QAbstractItemModel>
#include <QSharedPointer>

class Git;
class Lanes;
class Rev;
enum class RepositoryModelColumns;

class RepositoryModel : public QAbstractItemModel
{
   Q_OBJECT
public:
   RepositoryModel(QSharedPointer<Git> git, QObject *parent);
   ~RepositoryModel();
   void clear(bool complete = true);
   const QString sha(int row) const;
   int row(const QString &sha) const;
   const QStringList fileNames() const { return fNames; }
   void resetFileNames(const QString &fn);
   void setEarlyOutputState(bool b = true) { earlyOutputCnt = (b ? earlyOutputCntBase : -1); }
   void setAnnIdValid(bool b = true) { annIdValid = b; }

   virtual QVariant data(const QModelIndex &index, int role) const;
   virtual QVariant headerData(int s, Qt::Orientation o, int role = Qt::DisplayRole) const;
   virtual QModelIndex index(int r, int c, const QModelIndex &par = QModelIndex()) const;
   virtual QModelIndex parent(const QModelIndex &index) const;
   virtual int rowCount(const QModelIndex &par = QModelIndex()) const;
   virtual bool hasChildren(const QModelIndex &par = QModelIndex()) const;
   virtual int columnCount(const QModelIndex &) const { return mColumns.count(); }
   const Rev *revLookup(int row) const;

public slots:
   void on_changeFont(const QFont &);

private slots:
   void on_newRevsAdded();
   void on_loadCompleted(const QString &);

private:
   QSharedPointer<Git> mGit;
   friend class Git;

   void flushTail();
   const QString timeDiff(unsigned long secs) const;

   QHash<QString, const Rev *> revs;
   QVector<QString> revOrder;
   Lanes *lns = nullptr;
   uint firstFreeLane;
   QMap<RepositoryModelColumns, QString> mColumns;
   int rowCnt;
   bool annIdValid;
   unsigned long secs;
   int loadTime;
   int earlyOutputCnt;
   int earlyOutputCntBase;
   QStringList fNames;
   QStringList curFNames;
   QStringList renamedRevs;
   QHash<QString, QString> renamedPatches;
};
