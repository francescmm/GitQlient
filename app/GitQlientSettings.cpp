#include "GitQlientSettings.h"

void GitQlientSettings::setValue(const QString &key, const QVariant &value)
{
   QSettings::setValue(key, value);

   emit valueChanged(key, value);
}
