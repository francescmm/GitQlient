#pragma once

#include <QStringList>

struct WorkingDirInfo
{
   void clear();
   QString diffIndex;
   QString diffIndexCached;
   QStringList otherFiles;
};
