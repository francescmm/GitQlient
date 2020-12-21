#include <GitSubtree.h>
#include <GitBase.h>
#include <GitQlientSettings.h>

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

   GitQlientSettings settings;

   for (auto i = 0;; ++i)
   {
      const auto repo = settings.localValue(mGitBase->getGitQlientSettingsDir(), QString("Subtrees/%1.prefix").arg(i));

      if (repo.toString() == name)
      {
         settings.setLocalValue(mGitBase->getGitQlientSettingsDir(), QString("Subtrees/%1.url").arg(i), url);
         settings.setLocalValue(mGitBase->getGitQlientSettingsDir(), QString("Subtrees/%1.ref").arg(i), ref);

         auto cmd = QString("git subtree add --prefix=%1 %2 %3").arg(name, url, ref);

         if (squash)
            cmd.append(" --squash");

         auto ret = mGitBase->run(cmd);

         if (ret.output.toString().contains("Cannot"))
            ret.success = false;

         return ret;
      }
      else if (repo.toString().isEmpty())
      {
         settings.setLocalValue(mGitBase->getGitQlientSettingsDir(), QString("Subtrees/%1.prefix").arg(i), name);
         settings.setLocalValue(mGitBase->getGitQlientSettingsDir(), QString("Subtrees/%1.url").arg(i), url);
         settings.setLocalValue(mGitBase->getGitQlientSettingsDir(), QString("Subtrees/%1.ref").arg(i), ref);

         return { true, "" };
      }
   }

   return { false, "" };
}

GitExecResult GitSubtree::pull(const QString &url, const QString &ref, const QString &prefix) const
{
   QLog_Debug("UI", "Pulling a subtree");

   auto ret = mGitBase->run(QString("git subtree pull --prefix=%1 %2 %3").arg(prefix, url, ref));

   if (ret.output.toString().contains("Cannot"))
      ret.success = false;

   return ret;
}

GitExecResult GitSubtree::push(const QString &url, const QString &ref, const QString &prefix) const
{
   QLog_Debug("UI", "Pushing changes in a subtree");

   auto ret = mGitBase->run(QString("git subtree push --prefix=%1 %2 %3").arg(prefix, url, ref));

   if (ret.output.toString().contains("Cannot"))
      ret.success = false;

   return ret;
}

GitExecResult GitSubtree::merge(const QString &sha) const
{
   QLog_Debug("UI", "Merging changes from the remote of a subtree");

   auto ret = mGitBase->run(QString("git subtree merge %1").arg(sha));

   if (ret.output.toString().contains("Cannot"))
      ret.success = false;

   return ret;
}

GitExecResult GitSubtree::list() const
{
   QLog_Debug("UI", "Listing all subtrees");

   return mGitBase->run(QString("git log --pretty=format:%b --grep=git-subtree-dir"));
}
