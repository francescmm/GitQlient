/*
        Author: Marco Costalba (C) 2005-2007

Copyright: See COPYING file that comes with this distribution

                */
#include "domain.h"

#include <RevisionsCache.h>
#include <Revision.h>

Domain::Domain(QSharedPointer<RevisionsCache> revCache)
   : QObject()
   , mRevCache(revCache)
{
   st.clear();
   busy = linked = false;
}

Domain::~Domain()
{
   if (!parent())
      return;
}

void Domain::clear(bool complete)
{
   if (complete)
      st.clear();
}

void Domain::deleteWhenDone()
{
   on_deleteWhenDone();
}

void Domain::on_deleteWhenDone()
{
   deleteLater();
}

void Domain::unlinkDomain(Domain *d)
{

   d->linked = false;
   while (d->disconnect(SIGNAL(updateRequested(StateInfo)), this))
      ; // a signal is emitted for every connection you make,
        // so if you duplicate a connection, two signals will be emitted.
}

void Domain::linkDomain(Domain *d)
{

   unlinkDomain(d); // be sure only one connection is active
   connect(d, &Domain::updateRequested, this, &Domain::on_updateRequested);
   d->linked = true;
}

void Domain::on_updateRequested(StateInfo newSt)
{

   st = newSt;
   update(false);
}

bool Domain::flushQueue()
{
   // during dragging any state update is queued, so try to flush pending now

   if (!busy && st.flushQueue())
   {
      update(false);
      return true;
   }
   return false;
}

void Domain::update(bool fromMaster)
{
   if (busy)
      return;

   if (linked && !fromMaster)
   {
      // in this case let the update to fall down from master domain
      StateInfo tmp(st);
      st.rollBack(); // we don't want to filter out next update sent from master
      emit updateRequested(tmp);
      return;
   }

   busy = true;

   if (const auto r = mRevCache->revLookup(st.sha()))
      st.setIsMerge(r->parentsCount() > 1);

   st.setLock(true); // any state change will be queued now
   emit signalUpdated();
   st.commit();
   st.rollBack();

   st.setLock(false);
   busy = false;

   flushQueue();
}
