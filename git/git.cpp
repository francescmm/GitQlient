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

Git::Git()
   : GitBase()
{
}

Git::Git(const QString &workingDirectory)
   : GitBase(workingDirectory)
{
}

uint Git::checkRef(const QString &sha, uint mask) const
{
   const auto ref = mRevCache->getReference(sha);

   return ref.isValid() ? ref.type & mask : 0;
}

const QStringList Git::getRefNames(const QString &sha, uint mask) const
{
   QStringList result;
   if (!checkRef(sha, mask))
      return result;

   const auto rf = mRevCache->getReference(sha);

   if (mask & TAG)
      result << rf.tags;

   if (mask & BRANCH)
      result << rf.branches;

   if (mask & RMT_BRANCH)
      result << rf.remoteBranches;

   if (mask & REF)
      result << rf.refs;

   if (mask == APPLIED || mask == UN_APPLIED)
      result << QStringList(rf.stgitPatch);

   return result;
}

CommitInfo Git::getCommitInfo(const QString &sha) const
{
   return mRevCache->getCommitInfo(sha);
}

GitExecResult Git::getCommitDiff(const QString &sha, const QString &diffToSha)
{
   if (!sha.isEmpty())
   {
      QString runCmd;

      if (sha != ZERO_SHA)
      {
         runCmd = "git diff-tree --no-color -r --patch-with-stat -C -m ";

         if (mRevCache->getCommitInfo(sha).parentsCount() == 0)
            runCmd.append("--root ");

         runCmd.append(QString("%1 %2").arg(diffToSha, sha)); // diffToSha could be empty
      }
      else
         runCmd = "git diff-index --no-color -r -m --patch-with-stat HEAD";

      return run(runCmd);
   }
   return qMakePair(false, QString());
}

QString Git::getFileDiff(const QString &currentSha, const QString &previousSha, const QString &file)
{
   QByteArray output;
   const auto ret = run(QString("git diff -U15000 %1 %2 %3").arg(previousSha, currentSha, file));

   if (ret.first)
      return ret.second;

   return QString();
}

bool Git::checkoutFile(const QString &fileName)
{
   if (fileName.isEmpty())
      return false;

   return run(QString("git checkout %1").arg(fileName)).first;
}

GitExecResult Git::resetFile(const QString &fileName)
{
   return run(QString("git reset %1").arg(fileName));
}

GitExecResult Git::blame(const QString &file, const QString &commitFrom)
{
   return run(QString("git annotate %1 %2").arg(file, commitFrom));
}

GitExecResult Git::history(const QString &file)
{
   return run(QString("git log --follow --pretty=%H %1").arg(file));
}

QPair<QString, QString> Git::getSplitCommitMsg(const QString &sha)
{
   const auto c = mRevCache->getCommitInfo(sha);

   return qMakePair(c.shortLog(), c.longLog().trimmed());
}

QVector<QString> Git::getSubmodules()
{
   QVector<QString> submodulesList;
   const auto ret = run("git config --file .gitmodules --name-only --get-regexp path");
   if (ret.first)
   {
      const auto submodules = ret.second.split('\n');
      for (auto submodule : submodules)
         if (!submodule.isEmpty() && submodule != "\n")
            submodulesList.append(submodule.split('.').at(1));
   }

   return submodulesList;
}

bool Git::submoduleAdd(const QString &url, const QString &name)
{
   return run(QString("git submodule add %1 %2").arg(url).arg(name)).first;
}

bool Git::submoduleUpdate(const QString &)
{
   return run("git submodule update --init --recursive").first;
}

bool Git::submoduleRemove(const QString &)
{
   return false;
}

RevisionFile Git::getWipFiles()
{
   return mRevCache->getRevisionFile(ZERO_SHA);
}

RevisionFile Git::getCommitFiles(const QString &sha) const
{
   return mRevCache->getRevisionFile(sha);
}

RevisionFile Git::getDiffFiles(const QString &sha, const QString &diffToSha, bool allFiles)
{
   const auto r = mRevCache->getCommitInfo(sha);
   if (r.parentsCount() == 0)
      return RevisionFile();

   QString mySha;
   QString runCmd = QString("git diff-tree -C --no-color -r -m ");

   if (r.parentsCount() > 1 && diffToSha.isEmpty() && allFiles)
   {
      mySha = QString("ALL_MERGE_FILES" + QString(sha));
      runCmd.append(sha);
   }
   else if (!diffToSha.isEmpty() && (sha != ZERO_SHA))
   {
      mySha = sha;
      runCmd.append(diffToSha + " " + sha);
   }

   if (mRevCache->containsRevisionFile(mySha))
      return mRevCache->getRevisionFile(mySha);

   const auto ret = run(runCmd);

   return ret.first ? mRevCache->parseDiff(sha, ret.second) : RevisionFile();
}

bool Git::resetCommits(int parentDepth)
{
   QString runCmd("git reset --soft HEAD~");
   runCmd.append(QString::number(parentDepth));
   return run(runCmd).first;
}

GitExecResult Git::checkoutCommit(const QString &sha)
{
   return run(QString("git checkout %1").arg(sha));
}

GitExecResult Git::markFileAsResolved(const QString &fileName)
{
   const auto ret = run(QString("git add %1").arg(fileName));

   if (ret.first)
      updateWipRevision();

   return ret;
}

bool Git::pendingLocalChanges()
{
   return mRevCache->pendingLocalChanges();
}

GitExecResult Git::merge(const QString &into, QStringList sources)
{
   const auto ret = run(QString("git checkout -q %1").arg(into));

   if (!ret.first)
      return ret;

   return run(QString("git merge -q ") + sources.join(" "));
}

bool Git::updateIndex(const QStringList &selFiles)
{
   const auto files = getWipFiles(); // files != nullptr

   QStringList toAdd, toRemove;

   for (auto it : selFiles)
   {
      const auto idx = mRevCache->findFileIndex(files, it);

      idx != -1 && files.statusCmp(idx, RevisionFile::DELETED) ? toRemove << it : toAdd << it;
   }

   if (!toRemove.isEmpty() && !run("git rm --cached --ignore-unmatch -- " + quote(toRemove)).first)
      return false;

   if (!toAdd.isEmpty() && !run("git add -- " + quote(toAdd)).first)
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
   RevisionFile files = getWipFiles(); // files != nullptr
   QStringList notSel;
   for (auto i = 0; i < files.count(); ++i)
   {
      const QString &fp = files.getFile(i);
      if (selFiles.indexOf(fp) == -1 && files.statusCmp(i, RevisionFile::IN_INDEX))
         notSel.append(fp);
   }

   // call git reset to remove not selected files from index
   if ((!notSel.empty() && !run("git reset -- " + quote(notSel)).first) || !updateIndex(selFiles)
       || !run(QString("git commit" + cmtOptions + " -m \"%1\"").arg(msg)).first
       || (!notSel.empty() && !updateIndex(notSel)))
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
      const auto ret = run(QString("git format-patch -1 %1").arg(sha));

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

         QFile::rename(QString("%1/%2").arg(mWorkingDirectory, filename), QString("%1/%2").arg(mWorkingDirectory, newFileName));
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
   const auto ret = run(QString("%1 %2").arg(cmd, fileName));

   return ret.first;
}

GitExecResult Git::push(bool force)
{
   return run(QString("git push ").append(force ? QString("--force") : QString()));
}

GitExecResult Git::pushUpstream(const QString &branchName)
{
   return run(QString("git push --set-upstream origin %1").arg(branchName));
}

GitExecResult Git::pull()
{
   return run("git pull");
}

bool Git::fetch()
{
   return run("git fetch --all --tags --prune --force").first;
}

GitExecResult Git::cherryPickCommit(const QString &sha)
{
   return run(QString("git cherry-pick %1").arg(sha));
}

GitExecResult Git::pop() const
{
   return run("git stash pop");
}

GitExecResult Git::stash()
{
   return run("git stash");
}

GitExecResult Git::stashBranch(const QString &stashId, const QString &branchName)
{
   return run(QString("git stash branch %1 %2").arg(branchName, stashId));
}

GitExecResult Git::stashDrop(const QString &stashId)
{
   return run(QString("git stash drop -q %1").arg(stashId));
}

GitExecResult Git::stashClear()
{
   return run("git stash clear");
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

   return run(QString("git reset --%1 %2").arg(typeStr, sha)).first;
}

GitExecResult Git::createBranchFromAnotherBranch(const QString &oldName, const QString &newName)
{
   return run(QString("git branch %1 %2").arg(newName, oldName));
}

GitExecResult Git::createBranchAtCommit(const QString &commitSha, const QString &branchName)
{
   return run(QString("git branch %1 %2").arg(branchName, commitSha));
}

GitExecResult Git::checkoutRemoteBranch(const QString &branchName)
{
   return run(QString("git checkout -q %1").arg(branchName));
}

GitExecResult Git::checkoutNewLocalBranch(const QString &branchName)
{
   return run(QString("git checkout -b %1").arg(branchName));
}

GitExecResult Git::renameBranch(const QString &oldName, const QString &newName)
{
   return run(QString("git branch -m %1 %2").arg(oldName, newName));
}

GitExecResult Git::removeLocalBranch(const QString &branchName)
{
   return run(QString("git branch -D %1").arg(branchName));
}

GitExecResult Git::removeRemoteBranch(const QString &branchName)
{
   return run(QString("git push --delete origin %1").arg(branchName));
}

GitExecResult Git::getBranches()
{
   return run(QString("git branch -a"));
}

GitExecResult Git::getDistanceBetweenBranches(bool toMaster, const QString &right)
{
   const QString firstArg = toMaster ? QString::fromUtf8("--left-right") : QString::fromUtf8("");
   const QString gitCmd = QString("git rev-list %1 --count %2...%3")
                              .arg(firstArg)
                              .arg(toMaster ? QString::fromUtf8("origin/master") : QString::fromUtf8("origin/%3"))
                              .arg(right);

   return run(gitCmd);
}

GitExecResult Git::getBranchesOfCommit(const QString &sha)
{
   return run(QString("git branch --contains %1 --all").arg(sha));
}

GitExecResult Git::getLastCommitOfBranch(const QString &branch)
{
   auto ret = run(QString("git rev-parse %1").arg(branch));

   if (ret.first)
      ret.second.remove(ret.second.count() - 1, ret.second.count());

   return ret;
}

GitExecResult Git::prune()
{
   return run("git remote prune origin");
}

QString Git::getCurrentBranch() const
{
   const auto ret = run("git rev-parse --abbrev-ref HEAD");

   return ret.first ? ret.second.trimmed() : QString();
}

QVector<QString> Git::getTags() const
{
   const auto ret = run("git tag");

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

QVector<QString> Git::getLocalTags() const
{
   const auto ret = run("git push --tags --dry-run");

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

GitExecResult Git::addTag(const QString &tagName, const QString &tagMessage, const QString &sha)
{
   return run(QString("git tag -a %1 %2 -m \"%3\"").arg(tagName).arg(sha).arg(tagMessage));
}

GitExecResult Git::removeTag(const QString &tagName, bool remote)
{
   GitExecResult ret;

   if (remote)
      ret = run(QString("git push origin --delete %1").arg(tagName));

   if (!remote || (remote && ret.success))
      ret = run(QString("git tag -d %1").arg(tagName));

   return ret;
}

GitExecResult Git::pushTag(const QString &tagName)
{
   return run(QString("git push origin %1").arg(tagName));
}

GitExecResult Git::getTagCommit(const QString &tagName)
{
   const auto ret = run(QString("git rev-list -n 1 %1").arg(tagName));
   QString output = ret.second;

   if (ret.first)
   {
      output.remove(output.count() - 2, output.count() - 1);
   }

   return qMakePair(ret.first, output);
}

QVector<QString> Git::getStashes()
{
   const auto ret = run("git stash list");

   QVector<QString> stashes;

   if (ret.first)
   {
      const auto tagsTmp = ret.second.split("\n");

      for (auto tag : tagsTmp)
         if (tag != "\n" && !tag.isEmpty())
            stashes.append(tag);
   }

   return stashes;
}

bool Git::clone(const QString &url, const QString &fullPath)
{
   const auto asyncRun = new GitCloneProcess(mWorkingDirectory);
   connect(asyncRun, &GitCloneProcess::signalProgress, this, &Git::signalCloningProgress, Qt::DirectConnection);

   QString buffer;
   return asyncRun->run(QString("git clone --progress %1 %2").arg(url, fullPath), buffer);
}

bool Git::initRepo(const QString &fullPath)
{
   return run(QString("git init %1").arg(fullPath)).first;
}

GitUserInfo Git::getGlobalUserInfo() const
{
   GitUserInfo userInfo;

   const auto nameRequest = run("git config --get --global user.name");

   if (nameRequest.first)
      userInfo.mUserName = nameRequest.second.trimmed();

   const auto emailRequest = run("git config --get --global user.email");

   if (emailRequest.first)
      userInfo.mUserEmail = emailRequest.second.trimmed();

   return userInfo;
}

void Git::setGlobalUserInfo(const GitUserInfo &info)
{
   run(QString("git config --global user.name \"%1\"").arg(info.mUserName));
   run(QString("git config --global user.email %1").arg(info.mUserEmail));
}

GitUserInfo Git::getLocalUserInfo() const
{
   GitUserInfo userInfo;

   const auto nameRequest = run("git config --get --local user.name");

   if (nameRequest.first)
      userInfo.mUserName = nameRequest.second.trimmed();

   const auto emailRequest = run("git config --get --local user.email");

   if (emailRequest.first)
      userInfo.mUserEmail = emailRequest.second.trimmed();

   return userInfo;
}

void Git::setLocalUserInfo(const GitUserInfo &info)
{
   run(QString("git config --local user.name \"%1\"").arg(info.mUserName));
   run(QString("git config --local user.email %1").arg(info.mUserEmail));
}

int Git::totalCommits() const
{
   return mRevCache->count();
}

CommitInfo Git::getCommitInfoByRow(int row) const
{
   return mRevCache->getCommitInfoByRow(row);
}

CommitInfo Git::getCommitInfo(const QString &sha)
{
   return mRevCache->getCommitInfo(sha);
}

bool GitUserInfo::isValid() const
{
   return !mUserEmail.isNull() && !mUserEmail.isEmpty() && !mUserName.isNull() && !mUserName.isEmpty();
}

// CT TODO utility function; can go elsewhere
const QString Git::quote(const QStringList &sl)
{
   QString q(sl.join(QString("$%1$").arg(' ')));
   q.prepend("$").append("$");
   return q;
}
