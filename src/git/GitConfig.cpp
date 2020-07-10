#include "GitConfig.h"

#include <GitBase.h>
#include <GitCloneProcess.h>

#include <QLogger.h>
#include <BenchmarkTool.h>

using namespace QLogger;
using namespace Benchmarker;

bool GitUserInfo::isValid() const
{
   return !mUserEmail.isNull() && !mUserEmail.isEmpty() && !mUserName.isNull() && !mUserName.isEmpty();
}

GitConfig::GitConfig(QSharedPointer<GitBase> gitBase, QObject *parent)
   : QObject(parent)
   , mGitBase(gitBase)
{
}

GitUserInfo GitConfig::getGlobalUserInfo() const
{
   BenchmarkStart();

   GitUserInfo userInfo;

   QLog_Debug("Git", QString("Getting global user info"));

   const auto nameRequest = mGitBase->run("git config --get --global user.name");

   if (nameRequest.success)
      userInfo.mUserName = nameRequest.output.toString().trimmed();

   const auto emailRequest = mGitBase->run("git config --get --global user.email");

   if (emailRequest.success)
      userInfo.mUserEmail = emailRequest.output.toString().trimmed();

   BenchmarkEnd();

   return userInfo;
}

void GitConfig::setGlobalUserInfo(const GitUserInfo &info)
{
   BenchmarkStart();

   QLog_Debug("Git", QString("Setting global user info"));

   mGitBase->run(QString("git config --global user.name \"%1\"").arg(info.mUserName));
   mGitBase->run(QString("git config --global user.email %1").arg(info.mUserEmail));

   BenchmarkEnd();
}

GitExecResult GitConfig::setGlobalData(const QString &key, const QString &value)
{
   BenchmarkStart();

   QLog_Debug("Git", QString("Configuring global key {%1} with value {%2}").arg(key, value));

   const auto ret = mGitBase->run(QString("git config --global %1 \"%2\"").arg(key, value));

   BenchmarkEnd();

   return ret;
}

GitUserInfo GitConfig::getLocalUserInfo() const
{
   BenchmarkStart();

   QLog_Debug("Git", QString("Getting local user info"));

   GitUserInfo userInfo;

   const auto nameRequest = mGitBase->run("git config --get --local user.name");

   if (nameRequest.success)
      userInfo.mUserName = nameRequest.output.toString().trimmed();

   const auto emailRequest = mGitBase->run("git config --get --local user.email");

   if (emailRequest.success)
      userInfo.mUserEmail = emailRequest.output.toString().trimmed();

   BenchmarkEnd();

   return userInfo;
}

void GitConfig::setLocalUserInfo(const GitUserInfo &info)
{
   BenchmarkStart();

   QLog_Debug("Git", QString("Setting local user info"));

   mGitBase->run(QString("git config --local user.name \"%1\"").arg(info.mUserName));
   mGitBase->run(QString("git config --local user.email %1").arg(info.mUserEmail));

   BenchmarkEnd();
}

GitExecResult GitConfig::setLocalData(const QString &key, const QString &value)
{
   BenchmarkStart();

   QLog_Debug("Git", QString("Configuring local key {%1} with value {%2}").arg(key, value));

   const auto ret = mGitBase->run(QString("git config --local %1 \"%2\"").arg(key, value));

   BenchmarkEnd();

   return ret;
}

GitExecResult GitConfig::clone(const QString &url, const QString &fullPath)
{
   QLog_Debug("Git", QString("Starting the clone process for repo {%1} at {%2}.").arg(url, fullPath));

   const auto asyncRun = new GitCloneProcess(mGitBase->getWorkingDir());
   connect(asyncRun, &GitCloneProcess::signalProgress, this, &GitConfig::signalCloningProgress, Qt::DirectConnection);

   mGitBase->setWorkingDir(fullPath);

   return asyncRun->run(QString("git clone --progress %1 %2").arg(url, fullPath));
}

GitExecResult GitConfig::initRepo(const QString &fullPath)
{
   BenchmarkStart();

   QLog_Debug("Git", QString("Initializing a new repository at {%1}").arg(fullPath));

   const auto ret = mGitBase->run(QString("git init %1").arg(fullPath));

   if (ret.success)
      mGitBase->setWorkingDir(fullPath);

   BenchmarkEnd();

   return ret;
}

GitExecResult GitConfig::getLocalConfig() const
{
   BenchmarkStart();

   QLog_Debug("Git", QString("Getting local config"));

   const auto ret = mGitBase->run("git config --local --list");

   BenchmarkEnd();

   return ret;
}

GitExecResult GitConfig::getGlobalConfig() const
{
   BenchmarkStart();

   QLog_Debug("Git", QString("Getting global config"));

   const auto ret = mGitBase->run("git config --global --list");

   BenchmarkEnd();

   return ret;
}

GitExecResult GitConfig::getRemoteForBranch(const QString &branch)
{
   BenchmarkStart();

   QLog_Debug("Git", QString("Getting remote for branch {%1}.").arg(branch));

   const auto config = getLocalConfig();

   if (config.success)
   {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
      const auto values = config.output.toString().split('\n', Qt::SkipEmptyParts);
#else
      const auto values = config.output.toString().split('\n', QString::SkipEmptyParts);
#endif
      const auto configKey = QString("branch.%1.remote=").arg(branch);
      QString configValue;

      for (const auto &value : values)
      {
         if (value.startsWith(configKey))
         {
            configValue = value.split("=").last();
            break;
         }
      }

      if (!configValue.isEmpty())
      {
         BenchmarkEnd();
         return { true, configValue };
      }
   }

   BenchmarkEnd();
   return GitExecResult();
}
