#include "GitTags.h"

#include <GitBase.h>

#include <QLogger.h>

using namespace QLogger;

GitTags::GitTags(const QSharedPointer<GitBase> &gitBase)
   : mGitBase(gitBase)
{
}

QMap<QString, QString> GitTags::getRemoteTags() const
{

   QLog_Debug("Git", QString("Executing getRemoteTags"));

   const auto ret = mGitBase->run("git ls-remote --tags");

   QMap<QString, QString> tags;

   if (ret.success)
   {
      const auto tagsTmp = ret.output.toString().split("\n");

      for (const auto &tag : tagsTmp)
      {
         if (tag != "\n" && !tag.isEmpty() && tag.contains("^{}"))
         {
            const auto sha = tag.split('\t').constFirst();
            const auto tagName = tag.split('\t').last().remove("refs/tags/").remove("^{}");

            tags.insert(tagName, sha);
         }
      }
   }

   return tags;
}

QVector<QString> GitTags::getLocalTags() const
{

   QLog_Debug("Git", QString("Executing getLocalTags"));

   const auto ret = mGitBase->run("git push --tags --dry-run");

   QVector<QString> tags;

   if (ret.success)
   {
      const auto tagsTmp = ret.output.toString().split("\n");

      for (const auto &tag : tagsTmp)
         if (tag != "\n" && !tag.isEmpty() && tag.contains("[new tag]"))
            tags.append(tag.split(" -> ").last());
   }

   return tags;
}

GitExecResult GitTags::addTag(const QString &tagName, const QString &tagMessage, const QString &sha)
{

   QLog_Debug("Git", QString("Executing addTag: {%1}").arg(tagName));

   const auto ret = mGitBase->run(QString("git tag -a %1 %2 -m \"%3\"").arg(tagName, sha, tagMessage));

   return ret;
}

GitExecResult GitTags::removeTag(const QString &tagName, bool remote)
{

   QLog_Debug("Git", QString("Executing removeTag: {%1}").arg(tagName));

   GitExecResult ret;

   if (remote)
      ret = mGitBase->run(QString("git push origin --delete %1").arg(tagName));

   if (!remote || (remote && ret.success))
      ret = mGitBase->run(QString("git tag -d %1").arg(tagName));

   return ret;
}

GitExecResult GitTags::pushTag(const QString &tagName)
{

   QLog_Debug("Git", QString("Executing pushTag: {%1}").arg(tagName));

   const auto ret = mGitBase->run(QString("git push origin %1").arg(tagName));

   return ret;
}

GitExecResult GitTags::getTagCommit(const QString &tagName)
{

   QLog_Debug("Git", QString("Executing getTagCommit: {%1}").arg(tagName));

   const auto ret = mGitBase->run(QString("git rev-list -n 1 %1").arg(tagName));
   const auto output = ret.output.toString().trimmed();

   return qMakePair(ret.success, output);
}
