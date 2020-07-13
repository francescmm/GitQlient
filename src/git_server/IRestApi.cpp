#include <IRestApi.h>

#include <QNetworkAccessManager>

IRestApi::IRestApi(const ServerAuthentication &auth, QObject *parent)
   : QObject(parent)
   , mManager(new QNetworkAccessManager())
   , mAuth(auth)
{
   if (!mAuth.endpointUrl.endsWith("/"))
      mAuth.endpointUrl.append("/");
}
