#ifndef DOMAIN_H
#define DOMAIN_H

#include <QObject>
#include <StateInfo.h>

class Domain : public QObject
{
   Q_OBJECT

public:
   Domain();
   void update();

   StateInfo st;

public slots:
   virtual void clear();
};

#endif
