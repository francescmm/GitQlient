/*
        Author: Marco Costalba (C) 2005-2007

        Copyright: See COPYING file that comes with this distribution

*/
#ifndef DOMAIN_H
#define DOMAIN_H

#include <QObject>
#include <QSharedPointer>
#include <StateInfo.h>

class Domain;
class Git;

class Domain : public QObject
{
   Q_OBJECT
public:
   Domain(QSharedPointer<Git> git, bool isMain);
   virtual ~Domain();
   void deleteWhenDone(); // will delete when no more run() are pending
   bool isLinked() const { return linked; }
   virtual bool isMatch(const QString &) { return false; }
   void update(bool fromMaster, bool force);

   StateInfo st;

signals:
   void updateRequested(StateInfo newSt);
   void cancelDomainProcesses();
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
   QSharedPointer<Git> mGit;

   bool busy;

private:
   void populateState();
   bool flushQueue();

   int exDeleteRequest;
   int exCancelRequest;

   bool linked;
};

#endif
