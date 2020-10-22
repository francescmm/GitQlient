#include "GitSubmodules.h"

#include <GitBase.h>
#include <QLogger.h>

#include <QFile>
#include <QTextStream>
#include <QTemporaryFile>

using namespace QLogger;

GitSubmodules::GitSubmodules(const QSharedPointer<GitBase> &gitBase)
   : mGitBase(gitBase)
{
}

QVector<QString> GitSubmodules::getSubmodules()
{
   QLog_Debug("Git", QString("Executing getSubmodules"));

   QVector<QString> submodulesList;
   const auto ret = mGitBase->run("git config --file .gitmodules --name-only --get-regexp path");
   if (ret.success)
   {
      const auto submodules = ret.output.toString().split('\n');
      for (const auto &submodule : submodules)
         if (!submodule.isEmpty() && submodule != "\n")
            submodulesList.append(submodule.split('.').at(1));
   }

   return submodulesList;
}

bool GitSubmodules::submoduleAdd(const QString &url, const QString &name)
{
   QLog_Debug("Git", QString("Executing submoduleAdd: {%1} {%2}").arg(url, name));

   const auto ret = mGitBase->run(QString("git submodule add %1 %2").arg(url, name)).success;

   return ret;
}

bool GitSubmodules::submoduleUpdate(const QString &)
{
   QLog_Debug("Git", QString("Executing submoduleUpdate"));

   const auto ret = mGitBase->run("git submodule update --init --recursive").success;

   return ret;
}

bool GitSubmodules::submoduleRemove(const QString &submodule)
{
   auto ret = mGitBase->run(QString("git submodule deinit -f %1").arg(submodule));
   ret = mGitBase->run(QString("git rm -f --cached %1").arg(submodule));

   ret = mGitBase->run(QString("rm -rf %1/.git/modules/%2").arg(mGitBase->getWorkingDir(), submodule));

   QFile gitmodules(QString("%1/.gitmodules").arg(mGitBase->getWorkingDir()));
   QTemporaryFile gitTmp;

   if (gitmodules.open(QIODevice::ReadOnly) && gitTmp.open())
   {
      QTextStream out(&gitmodules);
      QTextStream in(&gitTmp);
      auto removed = false;

      while (!out.atEnd())
      {
         auto line = out.readLine();

         if (line.contains(QString("[submodule \"%1\"]").arg(submodule)))
         {
            out.readLine();
            out.readLine();
            removed = true;
         }
         else
            in << line.append('\n');
      }
      gitmodules.close();
      gitTmp.close();

      if (removed)
      {
         gitmodules.remove();
         gitTmp.copy(QString("%1/.gitmodules").arg(mGitBase->getWorkingDir()));
      }
   }

   return ret.success;
}
