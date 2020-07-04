#include "References.h"

void References::addReference(Type type, const QString &value)
{
   const auto references = mReferences.value(type, QStringList());

   if (!references.contains(value))
      mReferences[type].append(value);
}

QStringList References::getReferences(Type type) const
{
   return mReferences.value(type, QStringList());
}
