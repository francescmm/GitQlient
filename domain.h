/*
        Author: Marco Costalba (C) 2005-2007

        Copyright: See COPYING file that comes with this distribution

*/
#ifndef DOMAIN_H
#define DOMAIN_H

#include <QObject>
#include <QPointer>
#include <StateInfo.h>

class Domain;
class RepositoryModel;

class Domain : public QObject
{
   Q_OBJECT

signals:
   void signalUpdated();

public:
   Domain(QPointer<RepositoryModel> repositoryModel);
   virtual ~Domain();
   void deleteWhenDone(); // will delete when no more run() are pending
   bool isLinked() const { return linked; }
   virtual bool isMatch(const QString &) { return false; }
   void update(bool fromMaster);

   StateInfo st;

signals:
   void updateRequested(StateInfo newSt);
   void setTabText(QWidget *w, const QString &label);

public slots:
   virtual void clear(bool complete = true);

protected slots:
   void on_updateRequested(StateInfo newSt);
   void on_deleteWhenDone();

protected:
   virtual bool doUpdate(bool) { return true; }
   void linkDomain(Domain *d);
   void unlinkDomain(Domain *d);
   QPointer<RepositoryModel> mRepositoryModel;

   bool busy;

private:
   bool flushQueue();

   int exDeleteRequest;
   int exCancelRequest;

   bool linked;
};

#endif
