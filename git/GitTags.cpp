#include "GitTags.h"

#include <GitBase.h>

GitTags::GitTags(const QSharedPointer<GitBase> &gitBase)
   : mGitBase(gitBase)
{
}

QVector<QString> GitTags::getTags() const
{
   const auto ret = mGitBase->run("git tag");

   QVector<QString> tags;

   if (ret.first)
   {
      const auto tagsTmp = ret.second.split("\n");

      for (auto tag : tagsTmp)
         if (tag != "\n" && !tag.isEmpty())
            tags.append(tag);
   }

   return tags;
}

QVector<QString> GitTags::getLocalTags() const
{
   const auto ret = mGitBase->run("git push --tags --dry-run");

   QVector<QString> tags;

   if (ret.first)
   {
      const auto tagsTmp = ret.second.split("\n");

      for (auto tag : tagsTmp)
         if (tag != "\n" && !tag.isEmpty() && tag.contains("[new tag]"))
            tags.append(tag.split(" -> ").last());
   }

   return tags;
}

GitExecResult GitTags::addTag(const QString &tagName, const QString &tagMessage, const QString &sha)
{
   return mGitBase->run(QString("git tag -a %1 %2 -m \"%3\"").arg(tagName).arg(sha).arg(tagMessage));
}

GitExecResult GitTags::removeTag(const QString &tagName, bool remote)
{
   GitExecResult ret;

   if (remote)
      ret = mGitBase->run(QString("git push origin --delete %1").arg(tagName));

   if (!remote || (remote && ret.success))
      ret = mGitBase->run(QString("git tag -d %1").arg(tagName));

   return ret;
}

GitExecResult GitTags::pushTag(const QString &tagName)
{
   return mGitBase->run(QString("git push origin %1").arg(tagName));
}

GitExecResult GitTags::getTagCommit(const QString &tagName)
{
   const auto ret = mGitBase->run(QString("git rev-list -n 1 %1").arg(tagName));
   QString output = ret.second;

   if (ret.first)
   {
      output.remove(output.count() - 2, output.count() - 1);
   }

   return qMakePair(ret.first, output);
}
