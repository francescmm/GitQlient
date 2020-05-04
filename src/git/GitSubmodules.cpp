#include "GitSubmodules.h"

#include <GitBase.h>
#include <QLogger.h>

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

   return mGitBase->run(QString("git submodule add %1 %2").arg(url, name)).success;
}

bool GitSubmodules::submoduleUpdate(const QString &)
{
   QLog_Debug("Git", QString("Executing submoduleUpdate"));

   return mGitBase->run("git submodule update --init --recursive").success;
}

bool GitSubmodules::submoduleRemove(const QString &)
{
   return false;
}
