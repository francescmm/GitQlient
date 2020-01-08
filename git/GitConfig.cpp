#include "GitConfig.h"

#include <GitBase.h>

bool GitUserInfo::isValid() const
{
   return !mUserEmail.isNull() && !mUserEmail.isEmpty() && !mUserName.isNull() && !mUserName.isEmpty();
}

GitConfig::GitConfig(QSharedPointer<GitBase> gitBase)
   : mGitBase(gitBase)
{
}

GitUserInfo GitConfig::getGlobalUserInfo() const
{
   GitUserInfo userInfo;

   const auto nameRequest = mGitBase->run("git config --get --global user.name");

   if (nameRequest.first)
      userInfo.mUserName = nameRequest.second.trimmed();

   const auto emailRequest = mGitBase->run("git config --get --global user.email");

   if (emailRequest.first)
      userInfo.mUserEmail = emailRequest.second.trimmed();

   return userInfo;
}

void GitConfig::setGlobalUserInfo(const GitUserInfo &info)
{
   mGitBase->run(QString("git config --global user.name \"%1\"").arg(info.mUserName));
   mGitBase->run(QString("git config --global user.email %1").arg(info.mUserEmail));
}

GitUserInfo GitConfig::getLocalUserInfo() const
{
   GitUserInfo userInfo;

   const auto nameRequest = mGitBase->run("git config --get --local user.name");

   if (nameRequest.first)
      userInfo.mUserName = nameRequest.second.trimmed();

   const auto emailRequest = mGitBase->run("git config --get --local user.email");

   if (emailRequest.first)
      userInfo.mUserEmail = emailRequest.second.trimmed();

   return userInfo;
}

void GitConfig::setLocalUserInfo(const GitUserInfo &info)
{
   mGitBase->run(QString("git config --local user.name \"%1\"").arg(info.mUserName));
   mGitBase->run(QString("git config --local user.email %1").arg(info.mUserEmail));
}
