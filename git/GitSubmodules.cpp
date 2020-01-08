#include "GitSubmodules.h"

#include <GitBase.h>

GitSubmodules::GitSubmodules(const QSharedPointer<GitBase> &gitBase)
   : mGitBase(gitBase)
{
}

QVector<QString> GitSubmodules::getSubmodules()
{
   QVector<QString> submodulesList;
   const auto ret = mGitBase->run("git config --file .gitmodules --name-only --get-regexp path");
   if (ret.first)
   {
      const auto submodules = ret.second.split('\n');
      for (auto submodule : submodules)
         if (!submodule.isEmpty() && submodule != "\n")
            submodulesList.append(submodule.split('.').at(1));
   }

   return submodulesList;
}

bool GitSubmodules::submoduleAdd(const QString &url, const QString &name)
{
   return mGitBase->run(QString("git submodule add %1 %2").arg(url).arg(name)).first;
}

bool GitSubmodules::submoduleUpdate(const QString &)
{
   return mGitBase->run("git submodule update --init --recursive").first;
}

bool GitSubmodules::submoduleRemove(const QString &)
{
   return false;
}
