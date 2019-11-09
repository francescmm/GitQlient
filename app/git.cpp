/*
        Description: interface to git programs

        Author: Marco Costalba (C) 2005-2007

        Copyright: See COPYING file that comes with this distribution

*/
#include "git.h"

#include <RevisionsCache.h>
#include <Revision.h>
#include <StateInfo.h>
#include <lanes.h>

#include "RepositoryModel.h"
#include "GitSyncProcess.h"
#include "GitAsyncProcess.h"
#include "GitRequestorProcess.h"
#include "domain.h"

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

static const QString GIT_LOG_FORMAT = "%m%HX%PX%n%cn<%ce>%n%an<%ae>%n%at%n%s%n";
static const QString CUSTOM_SHA = "*** CUSTOM * CUSTOM * CUSTOM * CUSTOM **";

namespace
{
#ifndef Q_OS_WIN32
#   include <sys/types.h> // used by chmod()
#   include <sys/stat.h> // used by chmod()
#endif

bool writeToFile(const QString &fileName, const QString &data)
{

   QFile file(fileName);
   if (!file.open(QIODevice::WriteOnly))
      return false;

   QString data2(data);
   QTextStream stream(&file);

#ifdef Q_OS_WIN32
   data2.replace("\r\n", "\n"); // change windows CRLF to linux
   data2.replace("\n", "\r\n"); // then change all linux CRLF to windows
#endif
   stream << data2;
   file.close();

   return true;
}
}

Git::Git()
   : QObject()
{
}

void Git::userInfo(QStringList &info)
{
   /*
   git looks for commit user information in following order:

         - GIT_AUTHOR_NAME and GIT_AUTHOR_EMAIL environment variables
         - repository config file
         - global config file
         - your name, hostname and domain
 */
   const QString env(QProcess::systemEnvironment().join(","));
   QString user(env.section("GIT_AUTHOR_NAME", 1).section(",", 0, 0).section("=", 1).trimmed());
   QString email(env.section("GIT_AUTHOR_EMAIL", 1).section(",", 0, 0).section("=", 1).trimmed());

   info.clear();
   info << "Environment" << user << email;

   user = run("git config user.name").second;
   email = run("git config user.email").second;
   info << "Local config" << user << email;

   user = run("git config --global user.name").second;
   email = run("git config --global user.email").second;
   info << "Global config" << user << email;
}

// CT TODO utility function; can go elsewhere
const QString Git::quote(const QString &nm)
{

   return ("$" + nm + "$");
}

// CT TODO utility function; can go elsewhere
const QString Git::quote(const QStringList &sl)
{

   QString q(sl.join(QString("$%1$").arg(' ')));
   q.prepend("$").append("$");
   return q;
}

uint Git::checkRef(const QString &sha, uint mask) const
{

   QHash<QString, Reference>::const_iterator it(mRefsShaMap.constFind(sha));
   return (it != mRefsShaMap.constEnd() ? (*it).type & mask : 0);
}

const QStringList Git::getRefNames(const QString &sha, uint mask) const
{

   QStringList result;
   if (!checkRef(sha, mask))
      return result;

   const Reference &rf = mRefsShaMap[sha];

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

const QString Git::getRefSha(const QString &refName, RefType type, bool askGit)
{
   bool any = type == ANY_REF;

   for (auto it = mRefsShaMap.cbegin(); it != mRefsShaMap.cend(); ++it)
   {
      const Reference &rf = *it;

      if ((any || type == TAG) && rf.tags.contains(refName))
         return it.key();

      else if ((any || type == BRANCH) && rf.branches.contains(refName))
         return it.key();

      else if ((any || type == RMT_BRANCH) && rf.remoteBranches.contains(refName))
         return it.key();

      else if ((any || type == REF) && rf.refs.contains(refName))
         return it.key();

      else if ((any || type == APPLIED || type == UN_APPLIED) && rf.stgitPatch == refName)
         return it.key();
   }
   if (!askGit)
      return "";

   // if a ref was not found perhaps is an abbreviated form
   QString runOutput;
   const auto ret = run("git rev-parse --revs-only " + refName);
   return (ret.first ? ret.second.trimmed() : "");
}

const QString Git::getTagMsg(const QString &sha)
{

   if (!checkRef(sha, TAG))
      return "";

   Reference &rf = mRefsShaMap[sha];

   if (!rf.tagMsg.isEmpty())
      return rf.tagMsg;

   QRegExp pgp("-----BEGIN PGP SIGNATURE*END PGP SIGNATURE-----", Qt::CaseSensitive, QRegExp::Wildcard);

   if (!rf.tagObj.isEmpty())
   {
      auto ro = run("git cat-file tag " + rf.tagObj);
      if (ro.first)
         rf.tagMsg = ro.second.section("\n\n", 1).remove(pgp).trimmed();
   }
   return rf.tagMsg;
}

const QString Git::filePath(const RevisionFile &rf, int i) const
{
   return mDirNames[rf.dirAt(i)] + mFileNames[rf.nameAt(i)];
}

Revision Git::getRevLookup(const QString &sha) const
{
   return mRevCache->getRevLookup(sha);
}

QPair<bool, QString> Git::run(const QString &runCmd) const
{
   QString runOutput;
   GitSyncProcess p(mWorkingDir);
   connect(this, &Git::cancelAllProcesses, &p, &AGitProcess::onCancel);

   const auto ret = p.run(runCmd, runOutput);

   return qMakePair(ret, runOutput);
}

int Git::findFileIndex(const RevisionFile &rf, const QString &name)
{

   if (name.isEmpty())
      return -1;

   int idx = name.lastIndexOf('/') + 1;
   const QString &dr = name.left(idx);
   const QString &nm = name.mid(idx);

   for (int i = 0, cnt = rf.count(); i < cnt; ++i)
   {
      const auto isRevFile = mFileNames[rf.nameAt(i)];
      const auto isRevDir = mDirNames[rf.dirAt(i)];
      if (isRevFile == nm && isRevDir == dr)
         return i;
   }
   return -1;
}

const QStringList Git::getChildren(const QString &parent)
{

   QStringList children;
   const auto r = mRevCache->getRevLookup(parent);
   if (r.sha().isEmpty())
      return children;

   for (int i = 0; i < r.children.count(); i++)
      children.append(mRevCache->createRevisionSha(r.children[i]));

   // reorder children by loading order
   QStringList::iterator itC(children.begin());
   for (; itC != children.end(); ++itC)
   {
      const auto r = mRevCache->getRevLookup(*itC);
      (*itC).prepend(QString("%1 ").arg(r.orderIdx, 6));
   }
   children.sort();
   for (itC = children.begin(); itC != children.end(); ++itC)
      (*itC) = (*itC).section(' ', -1, -1);

   return children;
}

void Git::getDiff(const QString &sha, QObject *receiver, const QString &diffToSha, bool combined)
{
   if (!sha.isEmpty())
   {
      QString runCmd;
      if (sha != ZERO_SHA)
      {
         runCmd = "git diff-tree --no-color -r --patch-with-stat ";
         runCmd.append(combined ? QString("-c ") : QString("-C -m ")); // TODO rename for combined

         const auto r = mRevCache->getRevLookup(sha);
         if (r.parentsCount() == 0)
            runCmd.append("--root ");

         runCmd.append(diffToSha + " " + sha); // diffToSha could be empty
      }
      else
         runCmd = "git diff-index --no-color -r -m --patch-with-stat HEAD";

      auto p = new GitAsyncProcess(mWorkingDir, receiver);
      connect(this, &Git::cancelAllProcesses, p, &AGitProcess::onCancel);

      QString buf;
      if (!p->run(runCmd, buf))
         delete p;
   }
}

QString Git::getDiff(const QString &currentSha, const QString &previousSha, const QString &file)
{
   QByteArray output;
   const auto ret = run(QString("git diff -U15000 %1 %2 %3").arg(previousSha, currentSha, file));

   if (ret.first)
      return ret.second;

   return QString();
}

bool Git::isNothingToCommit()
{

   if (!mRevCache->containsRevisionFile(ZERO_SHA))
      return true;

   const auto rf = mRevCache->getRevisionFile(ZERO_SHA);
   return rf.count() == workingDirInfo.otherFiles.count();
}

bool Git::resetFile(const QString &fileName)
{
   if (fileName.isEmpty())
      return false;

   return run(QString("git checkout %1").arg(fileName)).first;
}

GitExecResult Git::blame(const QString &file)
{
   return run(QString("git annotate %1").arg(file));
}

GitExecResult Git::history(const QString &file)
{
   return run(QString("git log --follow --pretty=%H %1").arg(file));
}

QPair<QString, QString> Git::getSplitCommitMsg(const QString &sha)
{
   const auto c = mRevCache->getRevLookup(sha);

   return qMakePair(c.shortLog(), c.longLog().trimmed());
}

QString Git::getCommitMsg(const QString &sha) const
{
   const auto c = mRevCache->getRevLookup(sha);
   if (c.sha().isEmpty())
      return QString();

   return c.shortLog() + "\n\n" + c.longLog().trimmed();
}

const QString Git::getLastCommitMsg()
{
   // FIXME: Make sure the amend action is not called when there is
   // nothing to amend. That is in empty repository or over StGit stack
   // with nothing applied.
   QString sha;
   const auto ret = run("git rev-parse --verify HEAD");
   if (ret.first)
      sha = ret.second.trimmed();
   else
      return QString();

   return getCommitMsg(sha);
}

const QString Git::getNewCommitMsg()
{

   const auto c = mRevCache->getRevLookup(ZERO_SHA);
   if (c.sha().isEmpty())
      return "";

   QString status = c.longLog();
   status.prepend('\n').replace(QRegExp("\\n([^#\\n]?)"), "\n#\\1"); // comment all the lines

   if (mIsMergeHead)
   {
      QFile file(QDir(mGitDir).absoluteFilePath("MERGE_MSG"));
      if (file.open(QIODevice::ReadOnly))
      {
         QTextStream in(&file);

         while (!in.atEnd())
            status.prepend(in.readLine());

         file.close();
      }
   }
   return status;
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

RevisionFile Git::insertNewFiles(const QString &sha, const QString &data)
{

   /* we use an independent FileNamesLoader to avoid data
    * corruption if we are loading file names in background
    */
   FileNamesLoader fl;

   RevisionFile rf;
   parseDiffFormat(rf, data, fl);
   flushFileNames(fl);

   mRevCache->insertRevisionFile(sha, rf);

   return rf;
}

bool Git::runDiffTreeWithRenameDetection(const QString &runCmd, QString *runOutput)
{
   /* Under some cases git could warn out:

       "too many files, skipping inexact rename detection"

    So if this occurs fallback on NO rename detection.
 */
   QString cmd(runCmd); // runCmd must be without -C option
   cmd.replace("git diff-tree", "git diff-tree -C");

   const auto renameDetection = run(cmd);

   *runOutput = renameDetection.second;

   if (!renameDetection.first) // retry without rename detection
   {
      const auto ret2 = run(runCmd);
      if (ret2.first)
         *runOutput = ret2.second;

      return ret2.first;
   }

   return true;
}

RevisionFile Git::getWipFiles()
{
   return mRevCache->getRevisionFile(ZERO_SHA); // ZERO_SHA search arrives here
}

RevisionFile Git::getFiles(const QString &sha) const
{
   const auto r = mRevCache->getRevLookup(sha);

   if (r.parentsCount() != 0 && mRevCache->containsRevisionFile(sha))
      return mRevCache->getRevisionFile(sha);

   return RevisionFile();
}

RevisionFile Git::getFiles(const QString &sha, const QString &diffToSha, bool allFiles)
{
   const auto r = mRevCache->getRevLookup(sha);
   if (r.parentsCount() == 0)
      return RevisionFile();

   QString mySha;
   QString runCmd = QString("git diff-tree --no-color -r -m ");

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

   QString runOutput;

   if (runDiffTreeWithRenameDetection(runCmd, &runOutput))
      return insertNewFiles(mySha, runOutput);

   return RevisionFile();
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

GitExecResult Git::merge(const QString &into, QStringList sources)
{
   const auto ret = run(QString("git checkout -q %1").arg(into));

   if (!ret.first)
      return ret;

   return run(QString("git merge -q ") + sources.join(" "));
}

const QStringList Git::getOtherFiles(const QStringList &selFiles)
{

   RevisionFile files = getWipFiles(); // files != nullptr
   QStringList notSelFiles;
   for (auto i = 0; i < files.count(); ++i)
   {
      const QString &fp = filePath(files, i);
      if (selFiles.indexOf(fp) == -1 && files.statusCmp(i, RevisionFile::IN_INDEX))
         notSelFiles.append(fp);
   }
   return notSelFiles;
}

bool Git::updateIndex(const QStringList &selFiles)
{
   const auto files = getWipFiles(); // files != nullptr

   QStringList toAdd, toRemove;

   for (auto it : selFiles)
   {
      const auto idx = findFileIndex(files, it);

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
   const QString msgFile(mGitDir + "/qgit_cmt_msg.txt");
   if (!writeToFile(msgFile, msg)) // early skip
      return false;

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
   const QStringList notSel(getOtherFiles(selFiles));

   // call git reset to remove not selected files from index
   if ((!notSel.empty() && !run("git reset -- " + quote(notSel)).first) || !updateIndex(selFiles)
       || !run("git commit" + cmtOptions + " -F " + quote(msgFile)).first || (!notSel.empty() && !updateIndex(notSel)))
   {
      QDir dir(mWorkingDir);
      dir.remove(msgFile);
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

         QFile::rename(QString("%1/%2").arg(mWorkingDir, filename), QString("%1/%2").arg(mWorkingDir, newFileName));
         ++val;
      }
   }

   if (val != shaList.count())
      QLog_Error("Git", QString("Problem generating patches. Stop after {%1} iterations").arg(val));

   GitExecResult res;
   res.success = true;
   res.output = files;
   return res;
}

bool Git::apply(const QString &fileName, bool asCommit)
{
   const auto cmd = asCommit ? QString("git am --signof") : QString("git apply");
   const auto ret = run(QString("%1 %2").arg(cmd, fileName));

   return ret.first;
}

GitExecResult Git::push(bool force)
{
   QString output;
   const auto ret = run(QString("git push ").append(force ? QString("--force") : QString()));
   output = ret.second;

   if (output.contains("has no upstream branch"))
      return run(QString("git push --set-upstream origin %1").arg(mCurrentBranchName));

   return ret;
}

GitExecResult Git::pull()
{
   const auto ret = run("git pull");

   return ret;
}

bool Git::fetch()
{
   return run("git fetch --all --tags --prune --force").first;
}

GitExecResult Git::cherryPickCommit(const QString &sha)
{
   return run(QString("git cherry-pick %1").arg(sha));
}

bool Git::pop()
{
   return run("git stash pop").first;
}

bool Git::stash()
{
   return run("git stash").first;
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
      default:
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

bool Git::addTag(const QString &tagName, const QString &tagMessage, const QString &sha, QByteArray &output)
{
   const auto ret = run(QString("git tag -a %1 %2 -m \"%3\"").arg(tagName).arg(sha).arg(tagMessage));
   output = ret.second.toUtf8();

   return ret.first;
}

bool Git::removeTag(const QString &tagName, bool remote)
{
   auto ret = false;

   if (remote)
      ret = run(QString("git push origin --delete %1").arg(tagName)).first;

   if (!remote || (remote && ret))
      ret = run(QString("git tag -d %1").arg(tagName)).first;

   return ret;
}

bool Git::pushTag(const QString &tagName, QByteArray &output)
{
   const auto ret = run(QString("git push origin %1").arg(tagName));
   output = ret.second.toUtf8();

   return ret.first;
}

bool Git::getTagCommit(const QString &tagName, QByteArray &output)
{
   const auto ret = run(QString("git rev-list -n 1 %1").arg(tagName));
   output = ret.second.toUtf8();

   if (ret.first)
   {
      output.remove(output.count() - 2, output.count() - 1);
   }

   return ret.first;
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

bool Git::getStashCommit(const QString &stash, QByteArray &output)
{
   const auto ret = run(QString("git rev-list %1 -n 1 ").arg(stash));
   output = ret.second.toUtf8();

   if (ret.first)
      output.remove(output.count() - 2, output.count() - 1);

   return ret.first;
}

bool Git::getGitDBDir(const QString &wd)
{
   // we could run from a subdirectory, so we need to get correct directories

   QString tmp(mWorkingDir);
   mWorkingDir = wd;
   const auto success = run("git rev-parse --git-dir"); // run under newWorkDir
   mWorkingDir = tmp;
   const auto runOutput = success.second.trimmed();
   if (success.first)
   {
      // 'git rev-parse --git-dir' output could be a relative
      // to working directory (as ex .git) or an absolute path
      QDir d(runOutput.startsWith("/") ? runOutput : wd + "/" + runOutput);
      mGitDir = d.absolutePath();
   }
   return success.first;
}

bool Git::getBaseDir(const QString &wd, QString &bd)
{
   // we could run from a subdirectory, so we need to get correct directories

   // We use --show-cdup and not --git-dir for this, in order to take into account configurations
   //  in which .git is indeed a "symlink", a text file containing the path of the actual .git database dir.
   // In that particular case, the parent directory of the one given by --git-dir is *not* necessarily
   //  the base directory of the repository.

   QString tmp(mWorkingDir);
   mWorkingDir = wd;
   auto ret = run("git rev-parse --show-cdup"); // run under newWorkDir
   mWorkingDir = tmp;
   if (ret.first)
   {
      QDir d(QString("%1/%2").arg(wd, ret.second.trimmed()));
      bd = d.absolutePath();
   }
   else
      bd = wd;

   return ret.first;
}

Git::Reference *Git::lookupOrAddReference(const QString &sha)
{
   QHash<QString, Reference>::iterator it(mRefsShaMap.find(sha));
   if (it == mRefsShaMap.end())
      it = mRefsShaMap.insert(sha, Reference());
   return &(*it);
}

bool Git::getRefs()
{
   // check for a StGIT stack
   QDir d(mGitDir);

   // check for a merge and read current branch sha
   mIsMergeHead = d.exists("MERGE_HEAD");
   const auto ret = run("git rev-parse --revs-only HEAD");
   if (!ret.first)
      return false;

   QString curBranchSHA = ret.second;

   const auto ret2 = run("git branch");
   if (!ret2.first)
      return false;

   mCurrentBranchName = ret2.second;

   curBranchSHA = curBranchSHA.trimmed();
   mCurrentBranchName = mCurrentBranchName.prepend('\n').section("\n*", 1);
   mCurrentBranchName = mCurrentBranchName.section('\n', 0, 0).trimmed();
   if (mCurrentBranchName.contains(" detached "))
      mCurrentBranchName = "";

   // read refs, normally unsorted
   const auto ret3 = run("git show-ref -d");
   if (!ret3.first)
      return false;

   mRefsShaMap.clear();

   QString prevRefSha;
   QStringList patchNames, patchShas;
   const QStringList rLst(ret3.second.split('\n', QString::SkipEmptyParts));
   for (auto it : rLst)
   {

      const auto revSha = it.left(40);
      const auto refName = it.mid(41);

      // one Revision could have many tags
      Reference *cur = lookupOrAddReference(revSha);

      if (refName.startsWith("refs/tags/"))
      {

         if (refName.endsWith("^{}"))
         { // tag dereference

            // we assume that a tag dereference follows strictly
            // the corresponding tag object in rLst. So the
            // last added tag is a tag object, not a commit object
            cur->tags.append(refName.mid(10, refName.length() - 13));

            // store tag object. Will be used to fetching
            // tag message (if any) when necessary.
            cur->tagObj = prevRefSha;

            // tagObj must be removed from ref map
            if (!prevRefSha.isEmpty())
               mRefsShaMap.remove(prevRefSha);
         }
         else
            cur->tags.append(refName.mid(10));

         cur->type |= TAG;
      }
      else if (refName.startsWith("refs/heads/"))
      {

         cur->branches.append(refName.mid(11));
         cur->type |= BRANCH;
         if (curBranchSHA == revSha)
            cur->type |= CUR_BRANCH;
      }
      else if (refName.startsWith("refs/remotes/") && !refName.endsWith("HEAD"))
      {

         cur->remoteBranches.append(refName.mid(13));
         cur->type |= RMT_BRANCH;
      }
      else if (!refName.startsWith("refs/bases/") && !refName.endsWith("HEAD"))
      {

         cur->refs.append(refName);
         cur->type |= REF;
      }
      prevRefSha = revSha;
   }

   // mark current head (even when detached)
   auto cur = lookupOrAddReference(curBranchSHA);
   cur->type |= CUR_BRANCH;

   return !mRefsShaMap.empty();
}

const QStringList Git::getOthersFiles()
{
   // add files present in working directory but not in git archive

   QString runCmd("git ls-files --others");
   QString exFile(".git/info/exclude");
   QString path = QString("%1/%2").arg(mWorkingDir, exFile);

   if (QFile::exists(path))
      runCmd.append(" --exclude-from=" + quote(exFile));

   runCmd.append(" --exclude-per-directory=" + quote(".gitignore"));

   const auto runOutput = run(runCmd).second;
   return runOutput.split('\n', QString::SkipEmptyParts);
}

Revision Git::fakeRevData(const QString &sha, const QStringList &parents, const QString &author, const QString &date,
                          const QString &log, const QString &longLog, const QString &patch, int idx)
{

   QString data('>' + sha + 'X' + parents.join(" ") + " \n");
   data.append(author + '\n' + author + '\n' + date + '\n');
   data.append(log + '\n' + longLog);

   QString header("log size " + QString::number(QByteArray(data.toLatin1()).length() - 1) + '\n');
   data.prepend(header);
   if (!patch.isEmpty())
      data.append('\n' + patch);

   QTextCodec *tc = QTextCodec::codecForLocale();
   QByteArray *ba = new QByteArray(tc->fromUnicode(data));

   ba->append('\0');

   int dummy;
   return Revision(*ba, 0, idx, &dummy);
}

Revision Git::fakeWorkDirRev(const QString &parent, const QString &log, const QString &longLog, int idx)
{

   QString patch;
   QString date(QString::number(QDateTime::currentDateTime().toSecsSinceEpoch()));
   QString author("-");
   QStringList parents(parent);
   Revision c = fakeRevData(ZERO_SHA, parents, author, date, log, longLog, patch, idx);
   c.isDiffCache = true;
   c.lanes.append(LaneType::EMPTY);
   return c;
}

RevisionFile Git::fakeWorkDirRevFile(const WorkingDirInfo &wd)
{

   FileNamesLoader fl;
   RevisionFile rf;
   parseDiffFormat(rf, wd.diffIndex, fl);
   rf.onlyModified = false;

   for (auto it : wd.otherFiles)
   {

      appendFileName(rf, it, fl);
      rf.status.append(RevisionFile::UNKNOWN);
      rf.mergeParent.append(1);
   }
   RevisionFile cachedFiles;
   parseDiffFormat(cachedFiles, wd.diffIndexCached, fl);
   flushFileNames(fl);

   for (auto i = 0; i < rf.count(); i++)
      if (findFileIndex(cachedFiles, filePath(rf, i)) != -1)
         rf.status[i] |= RevisionFile::IN_INDEX;

   return rf;
}

void Git::updateWipRevision()
{

   const auto ret = run("git status");
   if (!ret.first) // git status refreshes the index, run as first
      return;

   QString status = ret.second;

   const auto ret2 = run("git rev-parse --revs-only HEAD");
   if (!ret2.first)
      return;

   QString head = ret2.second;

   head = head.trimmed();
   if (!head.isEmpty())
   { // repository initialized but still no history

      const auto ret3 = run("git diff-index " + head);

      if (!ret3.first)
         return;

      workingDirInfo.diffIndex = ret3.second;

      // check for files already updated in cache, we will
      // save this information in status third field
      const auto ret4 = run("git diff-index --cached " + head);
      if (!ret4.first)
         return;

      workingDirInfo.diffIndexCached = ret4.second;
   }
   // get any file not in tree
   workingDirInfo.otherFiles = getOthersFiles();

   // now mockup a RevisionFile
   mRevCache->insertRevisionFile(ZERO_SHA, fakeWorkDirRevFile(workingDirInfo));

   // then mockup the corresponding Revision
   const QString &log = (isNothingToCommit() ? QString("No local changes") : QString("Local changes"));
   auto r = fakeWorkDirRev(head, log, status, mRevCache->revOrderCount());

   mRevCache->insertRevision(ZERO_SHA, r);
}

void Git::parseDiffFormatLine(RevisionFile &rf, const QString &line, int parNum, FileNamesLoader &fl)
{

   if (line[1] == ':')
   { // it's a combined merge

      /* For combined merges rename/copy information is useless
       * because nor the original file name, nor similarity info
       * is given, just the status tracks that in the left/right
       * branch a renamed/copy occurred (as example status could
       * be RM or MR). For visualization purposes we could consider
       * the file as modified
       */
      appendFileName(rf, line.section('\t', -1), fl);
      setStatus(rf, "M");
      rf.mergeParent.append(parNum);
   }
   else
   { // faster parsing in normal case

      if (line.at(98) == '\t')
      {
         appendFileName(rf, line.mid(99), fl);
         setStatus(rf, line.at(97));
         rf.mergeParent.append(parNum);
      }
      else
         // it's a rename or a copy, we are not in fast path now!
         setExtStatus(rf, line.mid(97), parNum, fl);
   }
}

// CT TODO can go in RevisionFile
void Git::setStatus(RevisionFile &rf, const QString &rowSt)
{

   char status = rowSt.at(0).toLatin1();
   switch (status)
   {
      case 'M':
      case 'T':
      case 'U':
         rf.status.append(RevisionFile::MODIFIED);
         break;
      case 'D':
         rf.status.append(RevisionFile::DELETED);
         rf.onlyModified = false;
         break;
      case 'A':
         rf.status.append(RevisionFile::NEW);
         rf.onlyModified = false;
         break;
      case '?':
         rf.status.append(RevisionFile::UNKNOWN);
         rf.onlyModified = false;
         break;
      default:
         rf.status.append(RevisionFile::MODIFIED);
         break;
   }
}

void Git::setExtStatus(RevisionFile &rf, const QString &rowSt, int parNum, FileNamesLoader &fl)
{

   const QStringList sl(rowSt.split('\t', QString::SkipEmptyParts));
   if (sl.count() != 3)
      return;

   // we want store extra info with format "orig --> dest (Rxx%)"
   // but git give us something like "Rxx\t<orig>\t<dest>"
   QString type = sl[0];
   type.remove(0, 1);
   const QString &orig = sl[1];
   const QString &dest = sl[2];
   const QString extStatusInfo(orig + " --> " + dest + " (" + QString::number(type.toInt()) + "%)");

   /*
    NOTE: we set rf.extStatus size equal to position of latest
          copied/renamed file. So it can have size lower then
          rf.count() if after copied/renamed file there are
          others. Here we have no possibility to know final
          dimension of this RefFile. We are still in parsing.
 */

   // simulate new file
   appendFileName(rf, dest, fl);
   rf.mergeParent.append(parNum);
   rf.status.append(RevisionFile::NEW);
   rf.extStatus.resize(rf.status.size());
   rf.extStatus[rf.status.size() - 1] = extStatusInfo;

   // simulate deleted orig file only in case of rename
   if (type.at(0) == 'R')
   { // renamed file
      appendFileName(rf, orig, fl);
      rf.mergeParent.append(parNum);
      rf.status.append(RevisionFile::DELETED);
      rf.extStatus.resize(rf.status.size());
      rf.extStatus[rf.status.size() - 1] = extStatusInfo;
   }
   rf.onlyModified = false;
}

// CT TODO utility function; can go elsewhere
void Git::parseDiffFormat(RevisionFile &rf, const QString &buf, FileNamesLoader &fl)
{

   int parNum = 1, startPos = 0, endPos = buf.indexOf('\n');
   while (endPos != -1)
   {

      const QString &line = buf.mid(startPos, endPos - startPos);
      if (line[0] == ':') // avoid sha's in merges output
         parseDiffFormatLine(rf, line, parNum, fl);
      else
         parNum++;

      startPos = endPos + 1;
      endPos = buf.indexOf('\n', endPos + 99);
   }
}
bool Git::startRevList()
{
   QString baseCmd("git log --date-order --no-color "
#ifndef Q_OS_WIN32
                   "--log-size " // FIXME broken on Windows
#endif
                   "--parents --boundary -z "
                   "--pretty=format:"
                   + GIT_LOG_FORMAT);

   // we don't need log message body for file history
   baseCmd.append("%b --all");

   const auto requestor = new GitRequestorProcess(mWorkingDir);
   connect(requestor, &GitRequestorProcess::procDataReady, this, &Git::processInitLog);
   connect(this, &Git::cancelAllProcesses, requestor, &AGitProcess::onCancel);

   QString buf;
   const auto ret = requestor->run(baseCmd, buf);

   return ret;
}

bool Git::clone(const QString &url, const QString &fullPath)
{
   return run(QString("git clone %1 %2").arg(url, fullPath)).first;
}

bool Git::initRepo(const QString &fullPath)
{
   return run(QString("git init %1").arg(fullPath)).first;
}

void Git::clearRevs()
{
   mRevCache->clear();
   mRevCache->removeRevisionFile(ZERO_SHA);
   workingDirInfo.clear();
}

void Git::clearFileNames()
{
   mRevCache->clearRevisionFile();
   mFileNamesMap.clear();
   mDirNamesMap.clear();
   mDirNames.clear();
   mFileNames.clear();
}

bool Git::init(const QString &wd, QSharedPointer<RevisionsCache> revCache)
{
   QLog_Info("Git", "Initializing Git...");

   mRevCache = revCache;

   // normally called when changing git directory. Must be called after stop()
   clearRevs();

   const auto isGIT = getGitDBDir(wd);

   getBaseDir(wd, mWorkingDir);
   clearFileNames();

   if (!isGIT)
      return false;

   getRefs(); // load references

   QLog_Info("Git", "... Git init finished");

   return true;
}

void Git::init2()
{
   QLog_Info("Git", "Adding revisions...");

   updateWipRevision(); // blocking, we could be in setRepository() now

   startRevList();

   QLog_Info("Git", "... revisions finished");
}

void Git::processInitLog(const QByteArray &ba)
{
   int nextStart;
   auto start = 0;
   auto count = 0;

   do
   {
      // only here we create a new Revision
      Revision revision(ba, static_cast<uint>(start), mRevCache->revOrderCount(), &nextStart);
      // qDebug() << start << nextStart - start;
      // qDebug() << ba.mid(start, nextStart - start);
      start = nextStart;
      ++count;

      if (nextStart == -2)
      {
         start = ba.indexOf('\n', start) + 1;
         break;
      }
      else if (nextStart != -1)
      {
         const auto sha = revision.sha();

         if (!(revision.parentsCount() > 1 && mRevCache->contains(sha)))
            mRevCache->insertRevision(sha, revision);
      }
      else
         break;

   } while (nextStart < ba.size());
}

void Git::flushFileNames(FileNamesLoader &fl)
{

   if (!fl.rf)
      return;

   QByteArray &b = fl.rf->pathsIdx;
   QVector<int> &dirs = fl.rfDirs;

   b.clear();
   b.resize(2 * dirs.size() * static_cast<int>(sizeof(int)));

   int *d = (int *)(b.data());

   for (int i = 0; i < dirs.size(); i++)
   {

      d[i] = dirs.at(i);
      d[dirs.size() + i] = fl.rfNames.at(i);
   }
   dirs.clear();
   fl.rfNames.clear();
   fl.rf = nullptr;
}

void Git::appendFileName(RevisionFile &rf, const QString &name, FileNamesLoader &fl)
{

   if (fl.rf != &rf)
   {
      flushFileNames(fl);
      fl.rf = &rf;
   }
   int idx = name.lastIndexOf('/') + 1;
   const QString &dr = name.left(idx);
   const QString &nm = name.mid(idx);

   QHash<QString, int>::const_iterator it(mDirNamesMap.constFind(dr));
   if (it == mDirNamesMap.constEnd())
   {
      int idx = mDirNames.count();
      mDirNamesMap.insert(dr, idx);
      mDirNames.append(dr);
      fl.rfDirs.append(idx);
   }
   else
      fl.rfDirs.append(*it);

   it = mFileNamesMap.constFind(nm);
   if (it == mFileNamesMap.constEnd())
   {
      int idx = mFileNames.count();
      mFileNamesMap.insert(nm, idx);
      mFileNames.append(nm);
      fl.rfNames.append(idx);
   }
   else
      fl.rfNames.append(*it);
}
