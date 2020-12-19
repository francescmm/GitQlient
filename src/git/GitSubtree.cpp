#include <GitSubtree.h>
#include <GitBase.h>
#include <QLogger.h>

using namespace QLogger;

GitSubtree::GitSubtree(const QSharedPointer<GitBase> &gitBase)
   : QObject()
   , mGitBase(gitBase)
{
}

GitExecResult GitSubtree::add(const QString &url, const QString &ref, const QString &name, bool squash)
{
   QLog_Debug("UI", "Adding a subtree");

   auto cmd = QString("git subtree add --prefix=%1 %2 %3").arg(name, url, ref);

   if (squash)
      cmd.append(" --squash");

   return mGitBase->run(cmd);
}

GitExecResult GitSubtree::pull() const
{
   QLog_Debug("UI", "Pulling a subtree");

   return mGitBase->run(QString("git subtree pull"));
}

GitExecResult GitSubtree::push() const
{
   QLog_Debug("UI", "Pushing changes in a subtree");

   return mGitBase->run(QString("git subtree push"));
}

GitExecResult GitSubtree::merge(const QString &sha) const
{
   QLog_Debug("UI", "Merging changes from the remote of a subtree");

   return mGitBase->run(QString("git subtree merge %1").arg(sha));
}
