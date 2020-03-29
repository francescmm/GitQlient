#include "GitLocal.h"

#include <GitBase.h>
#include <QLogger.h>

using namespace QLogger;

namespace
{
static QString quote(const QStringList &sl)
{
   QString q(sl.join(QString("$%1$").arg(' ')));
   q.prepend("$").append("$");
   return q;
}
}

GitLocal::GitLocal(const QSharedPointer<GitBase> &gitBase)
   : QObject()
   , mGitBase(gitBase)
{
}

GitExecResult GitLocal::cherryPickCommit(const QString &sha) const
{
   QLog_Debug("Git", QString("Executing cherryPickCommit: {%1}").arg(sha));

   return mGitBase->run(QString("git cherry-pick %1").arg(sha));
}

GitExecResult GitLocal::cherryPickAbort() const
{
   QLog_Debug("Git", QString("Aborting cherryPick"));

   return mGitBase->run("git cherry-pick --abort");
}

GitExecResult GitLocal::checkoutCommit(const QString &sha) const
{
   QLog_Debug("Git", QString("Executing checkoutCommit: {%1}").arg(sha));

   return mGitBase->run(QString("git checkout %1").arg(sha));
}

GitExecResult GitLocal::markFileAsResolved(const QString &fileName) const
{
   QLog_Debug("Git", QString("Executing markFileAsResolved: {%1}").arg(fileName));

   const auto ret = mGitBase->run(QString("git add %1").arg(fileName));

   if (ret.first)
      emit signalWipUpdated();

   return ret;
}

bool GitLocal::checkoutFile(const QString &fileName) const
{
   if (fileName.isEmpty())
   {
      QLog_Warning("Git", QString("Executing checkoutFile with an empty file.").arg(fileName));

      return false;
   }

   QLog_Debug("Git", QString("Executing checkoutFile: {%1}").arg(fileName));

   return mGitBase->run(QString("git checkout %1").arg(fileName)).first;
}

GitExecResult GitLocal::resetFile(const QString &fileName) const
{
   QLog_Debug("Git", QString("Executing resetFile: {%1}").arg(fileName));

   return mGitBase->run(QString("git reset %1").arg(fileName));
}

bool GitLocal::resetCommit(const QString &sha, CommitResetType type) const
{
   QString typeStr;

   switch (type)
   {
      case CommitResetType::SOFT:
         typeStr = "soft";
         break;
      case CommitResetType::MIXED:
         typeStr = "mixed";
         break;
      case CommitResetType::HARD:
         typeStr = "hard";
         break;
   }

   QLog_Debug("Git", QString("Executing resetCommit: {%1} type {%2}").arg(sha, typeStr));

   return mGitBase->run(QString("git reset --%1 %2").arg(typeStr, sha)).first;
}

GitExecResult GitLocal::commitFiles(QStringList &selFiles, const RevisionFiles &allCommitFiles, const QString &msg,
                                    bool amend, const QString &author) const
{
   // add user selectable commit options
   QString cmtOptions;

   if (amend)
   {
      cmtOptions.append(" --amend");

      if (!author.isEmpty())
         cmtOptions.append(QString(" --author \"%1\"").arg(author));
   }

   QStringList notSel;

   for (auto i = 0; i < allCommitFiles.count(); ++i)
   {
      const QString &fp = allCommitFiles.getFile(i);
      if (selFiles.indexOf(fp) == -1 && allCommitFiles.statusCmp(i, RevisionFiles::IN_INDEX))
         notSel.append(fp);
   }

   if (!notSel.empty())
   {
      const auto ret = mGitBase->run("git reset -- " + quote(notSel));

      if (!ret.first)
         return ret;
   }

   // call git reset to remove not selected files from index
   const auto updIdx = updateIndex(allCommitFiles, selFiles);

   if (!updIdx.success)
      return updIdx;

   QLog_Debug("Git", QString("Executing commitFiles: mode {%1}").arg(amend ? QString("amend") : QString("normal")));

   return mGitBase->run(QString("git commit" + cmtOptions + " -m \"%1\"").arg(msg));
}

GitExecResult GitLocal::updateIndex(const RevisionFiles &files, const QStringList &selFiles) const
{
   QStringList toAdd, toRemove;

   for (auto file : selFiles)
   {
      const auto index = files.mFiles.indexOf(file);

      if (index != -1 && files.statusCmp(index, RevisionFiles::DELETED))
         toRemove << file;
      else
         toAdd << file;
   }

   if (!toRemove.isEmpty())
   {
      const auto ret = mGitBase->run("git rm --cached --ignore-unmatch -- " + quote(toRemove));

      if (!ret.first)
         return ret;
   }

   if (!toAdd.isEmpty())
   {
      const auto ret = mGitBase->run("git add -- " + quote(toAdd));

      if (!ret.first)
         return ret;
   }

   return GitExecResult(true, "Indexes updated");
}
