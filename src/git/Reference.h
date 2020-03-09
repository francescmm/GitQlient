#pragma once

#include <ReferenceType.h>

#include <QString>
#include <QStringList>

struct Reference
{
   Reference() = default;

   void configure(const QString &refName, bool isCurrentBranch, const QString &prevRefSha);
   bool isValid() const { return type != 0; }

   uint type = 0;
   QStringList branches;
   QStringList remoteBranches;
   QStringList tags;
   QStringList refs;
   QString tagObj;
   QString tagMsg;
   QString stgitPatch;
};
