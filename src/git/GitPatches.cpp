#include "GitPatches.h"

#include <GitBase.h>
#include <QLogger.h>
#include <QBenchmark.h>

using namespace QLogger;
using namespace QBenchmark;

#include <QFile>

GitPatches::GitPatches(const QSharedPointer<GitBase> &gitBase)
   : mGitBase(gitBase)
{
}

GitExecResult GitPatches::exportPatch(const QStringList &shaList)
{
   QBenchmarkStart();

   QLog_Debug("Git", QString("Executing exportPatch: {%1}").arg(shaList.join(",")));

   auto val = 1;
   QStringList files;

   for (const auto &sha : shaList)
   {
      const auto ret = mGitBase->run(QString("git format-patch -1 %1").arg(sha));

      if (!ret.success)
         break;
      else
      {
         auto filename = ret.output.toString();
         filename = filename.remove("\n");
         const auto text = filename.mid(filename.indexOf("-") + 1);
         const auto number = QString("%1").arg(val, 4, 10, QChar('0'));
         const auto newFileName = QString("%1-%2").arg(number, text);
         files.append(newFileName);

         QFile::rename(QString("%1/%2").arg(mGitBase->getWorkingDir(), filename),
                       QString("%1/%2").arg(mGitBase->getWorkingDir(), newFileName));
         ++val;
      }
   }

   if (val != shaList.count())
      QLog_Error("Git", QString("Problem generating patches. Stop after {%1} iterations").arg(val));

   QBenchmarkEnd();

   return qMakePair(true, QVariant(files));
}

bool GitPatches::applyPatch(const QString &fileName, bool asCommit)
{
   QBenchmarkStart();

   QLog_Debug("Git",
              QString("Executing applyPatch: {%1} %2").arg(fileName, asCommit ? QString("as commit.") : QString()));

   const auto cmd = asCommit ? QString("git am --signof") : QString("git apply");
   const auto ret = mGitBase->run(QString("%1 %2").arg(cmd, fileName));

   QBenchmarkEnd();

   return ret.success;
}
