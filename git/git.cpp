/*
        Description: interface to git programs

        Author: Marco Costalba (C) 2005-2007

        Copyright: See COPYING file that comes with this distribution

*/
#include "git.h"

#include <RevisionsCache.h>
#include <CommitInfo.h>
#include <lanes.h>
#include <GitSyncProcess.h>
#include <GitCloneProcess.h>
#include <GitRequestorProcess.h>
#include <GitBase.h>

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QImageReader>
#include <QPalette>
#include <QRegExp>
#include <QSettings>
#include <QTextCodec>
#include <QTextDocument>
#include <QTextStream>
#include <QDateTime>

#include <QLogger.h>

using namespace QLogger;

Git::Git(const QSharedPointer<GitBase> &gitBase, QSharedPointer<RevisionsCache> cache, QObject *parent)
   : QObject(parent)
   , mGitBase(gitBase)
   , mCache(cache)
{
}

GitExecResult Git::getCommitDiff(const QString &sha, const QString &diffToSha)
{
   if (!sha.isEmpty())
   {
      QString runCmd = QString("git diff-tree --no-color -r --patch-with-stat -m");

      if (sha != CommitInfo::ZERO_SHA)
      {
         runCmd += " -C ";

         if (diffToSha.isEmpty())
            runCmd += " --root ";

         runCmd.append(QString("%1 %2").arg(diffToSha, sha)); // diffToSha could be empty
      }
      else
         runCmd += " HEAD ";

      return mGitBase->run(runCmd);
   }
   return qMakePair(false, QString());
}

QString Git::getFileDiff(const QString &currentSha, const QString &previousSha, const QString &file)
{
   QByteArray output;
   const auto ret = mGitBase->run(QString("git diff -U15000 %1 %2 %3").arg(previousSha, currentSha, file));

   if (ret.first)
      return ret.second;

   return QString();
}

bool Git::checkoutFile(const QString &fileName)
{
   if (fileName.isEmpty())
      return false;

   return mGitBase->run(QString("git checkout %1").arg(fileName)).first;
}

GitExecResult Git::resetFile(const QString &fileName)
{
   return mGitBase->run(QString("git reset %1").arg(fileName));
}

GitExecResult Git::blame(const QString &file, const QString &commitFrom)
{
   return mGitBase->run(QString("git annotate %1 %2").arg(file, commitFrom));
}

GitExecResult Git::history(const QString &file)
{
   return mGitBase->run(QString("git log --follow --pretty=%H %1").arg(file));
}

RevisionFile Git::getDiffFiles(const QString &sha, const QString &diffToSha)
{
   const auto r = mCache->getCommitInfo(sha);
   if (r.parentsCount() == 0)
      return RevisionFile();

   if (mCache->containsRevisionFile(sha))
      return mCache->getRevisionFile(sha);

   QString runCmd = QString("git diff-tree -C --no-color -r -m ");

   if (!diffToSha.isEmpty() && sha != CommitInfo::ZERO_SHA)
      runCmd.append(diffToSha + " " + sha);

   const auto ret = mGitBase->run(runCmd);

   return ret.first ? mCache->parseDiff(sha, ret.second) : RevisionFile();
}

bool Git::resetCommits(int parentDepth)
{
   QString runCmd("git reset --soft HEAD~");
   runCmd.append(QString::number(parentDepth));
   return mGitBase->run(runCmd).first;
}

GitExecResult Git::checkoutCommit(const QString &sha)
{
   return mGitBase->run(QString("git checkout %1").arg(sha));
}

GitExecResult Git::markFileAsResolved(const QString &fileName)
{
   const auto ret = mGitBase->run(QString("git add %1").arg(fileName));

   if (ret.first)
      emit signalWipUpdated();

   return ret;
}

GitExecResult Git::merge(const QString &into, QStringList sources)
{
   const auto ret = mGitBase->run(QString("git checkout -q %1").arg(into));

   if (!ret.first)
      return ret;

   return mGitBase->run(QString("git merge -q ") + sources.join(" "));
}

QString Git::getWorkingDir() const
{
   return mGitBase->getWorkingDir();
}

bool Git::updateIndex(const RevisionFile &files, const QStringList &selFiles)
{
   QStringList toAdd, toRemove;

   for (auto it : selFiles)
   {
      const auto idx = mCache->findFileIndex(files, it);

      idx != -1 && files.statusCmp(idx, RevisionFile::DELETED) ? toRemove << it : toAdd << it;
   }

   if (!toRemove.isEmpty() && !mGitBase->run("git rm --cached --ignore-unmatch -- " + quote(toRemove)).first)
      return false;

   if (!toAdd.isEmpty() && !mGitBase->run("git add -- " + quote(toAdd)).first)
      return false;

   return true;
}

bool Git::commitFiles(QStringList &selFiles, const QString &msg, bool amend, const QString &author)
{
   // add user selectable commit options
   QString cmtOptions;

   if (amend)
   {
      cmtOptions.append(" --amend");

      if (!author.isEmpty())
         cmtOptions.append(QString(" --author \"%1\"").arg(author));
   }

   bool ret = true;

   // get not selected files but updated in index to restore at the end
   const auto files = mCache->getRevisionFile(CommitInfo::ZERO_SHA);
   QStringList notSel;
   for (auto i = 0; i < files.count(); ++i)
   {
      const QString &fp = files.getFile(i);
      if (selFiles.indexOf(fp) == -1 && files.statusCmp(i, RevisionFile::IN_INDEX))
         notSel.append(fp);
   }

   // call git reset to remove not selected files from index
   const auto updIdx = updateIndex(files, selFiles);
   if ((!notSel.empty() && !mGitBase->run("git reset -- " + quote(notSel)).first) || !updIdx
       || !mGitBase->run(QString("git commit" + cmtOptions + " -m \"%1\"").arg(msg)).first
       || (!notSel.empty() && !updIdx))
   {
      ret = false;
   }

   return ret;
}

GitExecResult Git::exportPatch(const QStringList &shaList)
{
   auto val = 1;
   QStringList files;

   for (const auto &sha : shaList)
   {
      const auto ret = mGitBase->run(QString("git format-patch -1 %1").arg(sha));

      if (!ret.first)
         break;
      else
      {
         auto filename = ret.second;
         filename = filename.remove("\n");
         const auto text = filename.mid(filename.indexOf("-") + 1);
         const auto number = QString("%1").arg(val, 4, 10, QChar('0'));
         const auto newFileName = QString("%1-%2").arg(number, text);
         files.append(newFileName);

         QFile::rename(QString("%1/%2").arg(mGitBase->getWorkingDir(), filename),
                       QString("%1/%2").arg(mGitBase->getWorkingDir(), newFileName));
         ++val;
      }
   }

   if (val != shaList.count())
      QLog_Error("Git", QString("Problem generating patches. Stop after {%1} iterations").arg(val));

   return qMakePair(true, QVariant(files));
}

bool Git::apply(const QString &fileName, bool asCommit)
{
   const auto cmd = asCommit ? QString("git am --signof") : QString("git apply");
   const auto ret = mGitBase->run(QString("%1 %2").arg(cmd, fileName));

   return ret.first;
}

GitExecResult Git::push(bool force)
{
   return mGitBase->run(QString("git push ").append(force ? QString("--force") : QString()));
}

GitExecResult Git::pushUpstream(const QString &branchName)
{
   return mGitBase->run(QString("git push --set-upstream origin %1").arg(branchName));
}

GitExecResult Git::pull()
{
   return mGitBase->run("git pull");
}

bool Git::fetch()
{
   return mGitBase->run("git fetch --all --tags --prune --force").first;
}

GitExecResult Git::cherryPickCommit(const QString &sha)
{
   return mGitBase->run(QString("git cherry-pick %1").arg(sha));
}

GitExecResult Git::pop() const
{
   return mGitBase->run("git stash pop");
}

GitExecResult Git::stash()
{
   return mGitBase->run("git stash");
}

GitExecResult Git::stashBranch(const QString &stashId, const QString &branchName)
{
   return mGitBase->run(QString("git stash branch %1 %2").arg(branchName, stashId));
}

GitExecResult Git::stashDrop(const QString &stashId)
{
   return mGitBase->run(QString("git stash drop -q %1").arg(stashId));
}

GitExecResult Git::stashClear()
{
   return mGitBase->run("git stash clear");
}

bool Git::resetCommit(const QString &sha, Git::CommitResetType type)
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

   return mGitBase->run(QString("git reset --%1 %2").arg(typeStr, sha)).first;
}

GitExecResult Git::createBranchFromAnotherBranch(const QString &oldName, const QString &newName)
{
   return mGitBase->run(QString("git branch %1 %2").arg(newName, oldName));
}

GitExecResult Git::createBranchAtCommit(const QString &commitSha, const QString &branchName)
{
   return mGitBase->run(QString("git branch %1 %2").arg(branchName, commitSha));
}

GitExecResult Git::checkoutRemoteBranch(const QString &branchName)
{
   return mGitBase->run(QString("git checkout -q %1").arg(branchName));
}

GitExecResult Git::checkoutNewLocalBranch(const QString &branchName)
{
   return mGitBase->run(QString("git checkout -b %1").arg(branchName));
}

GitExecResult Git::renameBranch(const QString &oldName, const QString &newName)
{
   return mGitBase->run(QString("git branch -m %1 %2").arg(oldName, newName));
}

GitExecResult Git::removeLocalBranch(const QString &branchName)
{
   return mGitBase->run(QString("git branch -D %1").arg(branchName));
}

GitExecResult Git::removeRemoteBranch(const QString &branchName)
{
   return mGitBase->run(QString("git push --delete origin %1").arg(branchName));
}

GitExecResult Git::getBranchesOfCommit(const QString &sha)
{
   return mGitBase->run(QString("git branch --contains %1 --all").arg(sha));
}

GitExecResult Git::getLastCommitOfBranch(const QString &branch)
{
   auto ret = mGitBase->run(QString("git rev-parse %1").arg(branch));

   if (ret.first)
      ret.second.remove(ret.second.count() - 1, ret.second.count());

   return ret;
}

GitExecResult Git::prune()
{
   return mGitBase->run("git remote prune origin");
}

QString Git::getCurrentBranch() const
{
   return mGitBase->getCurrentBranch();
}

// CT TODO utility function; can go elsewhere
const QString Git::quote(const QStringList &sl)
{
   QString q(sl.join(QString("$%1$").arg(' ')));
   q.prepend("$").append("$");
   return q;
}
