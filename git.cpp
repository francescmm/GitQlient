/*
        Description: interface to git programs

        Author: Marco Costalba (C) 2005-2007

        Copyright: See COPYING file that comes with this distribution

*/
#include "git.h"

#include <RevisionsCache.h>
#include <Revision.h>
#include <RevisionFile.h>
#include <StateInfo.h>
#include <lanes.h>

#include "RepositoryModel.h"
#include "dataloader.h"
#include "lanes.h"
#include "GitSyncProcess.h"
#include "GitAsyncProcess.h"
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

#include <QLogger.h>

using namespace QLogger;

static const QString GIT_LOG_FORMAT = "%m%HX%PX%n%cn<%ce>%n%an<%ae>%n%at%n%s%n";
static const QString CUSTOM_SHA = "*** CUSTOM * CUSTOM * CUSTOM * CUSTOM **";
static const uint C_MAGIC = 0xA0B0C0D0;
static const int C_VERSION = 15;

const QString Git::kCacheFileName = QString("qgit_cache.dat");

namespace
{
const QString toPersistentSha(const QString &sha, QVector<QByteArray> &v)
{

   v.append(sha.toLatin1());
   return QString::fromUtf8(v.last().constData());
}

#ifndef Q_OS_WIN32
#   include <sys/types.h> // used by chmod()
#   include <sys/stat.h> // used by chmod()
#endif

bool writeToFile(const QString &fileName, const QString &data, bool setExecutable = false)
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

#ifndef Q_OS_WIN32
   if (setExecutable)
      chmod(fileName.toLatin1().constData(), 0755);
#endif
   return true;
}
}

Git::Git()
   : QObject()
{
   mRevsFiles.reserve(RevisionsCache::MAX_DICT_SIZE);
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

void Git::appendNamesWithId(QStringList &names, const QString &sha, const QStringList &data, bool onlyLoaded)
{

   const Revision *r = mRevCache->revLookup(sha);
   if (onlyLoaded && !r)
      return;

   if (onlyLoaded)
   { // prepare for later sorting
      const auto cap = QString("%1 ").arg(r->orderIdx, 6);

      for (auto it : data)
         names.append(cap + it);
   }
   else
      names += data;
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

void Git::addExtraFileInfo(QString *rowName, const QString &sha, const QString &diffToSha, bool allMergeFiles)
{

   const RevisionFile *files = getFiles(sha, diffToSha, allMergeFiles);
   if (!files)
      return;

   int idx = findFileIndex(*files, *rowName);
   if (idx == -1)
      return;

   QString extSt(files->extendedStatus(idx));
   if (extSt.isEmpty())
      return;

   *rowName = extSt;
}

void Git::removeExtraFileInfo(QString *rowName)
{

   if (rowName->contains(" --> ")) // return destination file name
      *rowName = rowName->section(" --> ", 1, 1).section(" (", 0, 0);
}

void Git::formatPatchFileHeader(QString *rowName, const QString &sha, const QString &diffToSha, bool combined,
                                bool allMergeFiles)
{
   if (combined)
   {
      rowName->prepend("diff --combined ");
      return; // TODO rename/copy still not supported in this case
   }
   // let's see if it's a rename/copy...
   addExtraFileInfo(rowName, sha, diffToSha, allMergeFiles);

   if (rowName->contains(" --> "))
   { // ...it is!

      const QString &destFile(rowName->section(" --> ", 1, 1).section(" (", 0, 0));
      const QString &origFile(rowName->section(" --> ", 0, 0));
      *rowName = "diff --git a/" + origFile + " b/" + destFile;
   }
   else
      *rowName = "diff --git a/" + *rowName + " b/" + *rowName;
}

const QString Git::filePath(const RevisionFile &rf, int i) const
{
   return mDirNames[rf.dirAt(i)] + mFileNames[rf.nameAt(i)];
}

void Git::cancelDataLoading()
{
   // normally called when closing file viewer

   emit cancelLoading(); // non blocking
}

const Revision *Git::revLookup(const QString &sha) const
{
   return mRevCache->revLookup(sha);
}

QPair<bool, QString> Git::run(const QString &runCmd) const
{
   QString runOutput;
   GitSyncProcess p(mWorkingDir);
   connect(this, &Git::cancelAllProcesses, &p, &AGitProcess::onCancel);

   const auto ret = p.run(runCmd, runOutput);

   return qMakePair(ret, runOutput);
}

void Git::runAsync(const QString &runCmd, QObject *receiver, const QString &buf)
{
   auto p = new GitAsyncProcess(mWorkingDir, receiver);
   connect(this, &Git::cancelAllProcesses, p, &AGitProcess::onCancel);

   if (!p->run(runCmd, const_cast<QString &>(buf)))
      delete p;
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

const QString Git::getLaneParent(const QString &fromSHA, int laneNum)
{

   const Revision *rs = mRevCache->revLookup(fromSHA);
   if (!rs)
      return "";

   for (int idx = rs->orderIdx - 1; idx >= 0; idx--)
   {

      const auto r = mRevCache->revLookup(mRevCache->createRevisionSha(idx));
      if (laneNum >= r->lanes.count())
         return "";

      if (!isFreeLane(static_cast<LaneType>(r->lanes[laneNum])))
      {

         auto type = r->lanes[laneNum];
         auto parNum = 0;
         while (!isMerge(type) && type != LaneType::ACTIVE)
         {

            if (isHead(static_cast<LaneType>(type)))
               parNum++;

            type = r->lanes[--laneNum];
         }
         return r->parent(parNum);
      }
   }
   return "";
}

const QStringList Git::getChildren(const QString &parent)
{

   QStringList children;
   const Revision *r = mRevCache->revLookup(parent);
   if (!r)
      return children;

   for (int i = 0; i < r->children.count(); i++)
      children.append(mRevCache->createRevisionSha(r->children[i]));

   // reorder children by loading order
   QStringList::iterator itC(children.begin());
   for (; itC != children.end(); ++itC)
   {
      const Revision *r = mRevCache->revLookup(*itC);
      (*itC).prepend(QString("%1 ").arg(r->orderIdx, 6));
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

         const auto r = mRevCache->revLookup(sha);
         if (r && r->parentsCount() == 0)
            runCmd.append("--root ");

         runCmd.append(diffToSha + " " + sha); // diffToSha could be empty
      }
      else
         runCmd = "git diff-index --no-color -r -m --patch-with-stat HEAD";

      runAsync(runCmd, receiver);
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

const QString Git::getWorkDirDiff(const QString &fileName)
{

   QString runCmd("git diff-index --no-color -r -z -m -p --full-index --no-commit-id HEAD"), runOutput;
   if (!fileName.isEmpty())
      runCmd.append(" -- " + quote(fileName));

   const auto ret = run(runCmd);
   if (!ret.first)
      return "";

   runOutput = ret.second;

   /* For unknown reasons file sha of index is not ZERO_SHA but
    a value of unknown origin.
    Replace that with ZERO_SHA so to not fool annotate
 */
   int idx = runOutput.indexOf("..");
   if (idx != -1)
      runOutput.replace(idx + 2, 40, ZERO_SHA);

   return runOutput;
}

bool Git::isNothingToCommit()
{

   if (!mRevsFiles.contains(ZERO_SHA))
      return true;

   const RevisionFile *rf = mRevsFiles[ZERO_SHA];
   return rf->count() == workingDirInfo.otherFiles.count();
}

bool Git::resetFile(const QString &fileName)
{
   if (fileName.isEmpty())
      return false;

   return run(QString("git checkout %1").arg(fileName)).first;
}

QPair<QString, QString> Git::getSplitCommitMsg(const QString &sha)
{
   const Revision *c = mRevCache->revLookup(sha);

   if (!c)
      return qMakePair(QString(), QString());

   return qMakePair(c->shortLog(), c->longLog().trimmed());
}

QString Git::getCommitMsg(const QString &sha) const
{
   const Revision *c = mRevCache->revLookup(sha);
   if (!c)
      return "";

   return c->shortLog() + "\n\n" + c->longLog().trimmed();
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
      return "";

   const Revision *c = mRevCache->revLookup(sha);

   if (!c)
      return "";

   return c->shortLog() + "\n\n" + c->longLog().trimmed();
}

const QString Git::getNewCommitMsg()
{

   const Revision *c = mRevCache->revLookup(ZERO_SHA);
   if (!c)
      return "";

   QString status = c->longLog();
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

const RevisionFile *Git::insertNewFiles(const QString &sha, const QString &data)
{

   /* we use an independent FileNamesLoader to avoid data
    * corruption if we are loading file names in background
    */
   FileNamesLoader fl;

   RevisionFile *rf = new RevisionFile();
   parseDiffFormat(*rf, data, fl);
   flushFileNames(fl);

   mRevsFiles.insert(toPersistentSha(sha, mRevsFilesShaBackupBuf), rf);
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

const RevisionFile *Git::getAllMergeFiles(const Revision *r)
{

   QString mySha("ALL_MERGE_FILES" + QString(r->sha()));

   if (mRevsFiles.contains(mySha))
      return mRevsFiles[mySha];

   QString runCmd(QString("git diff-tree --no-color -r -m ").arg(r->sha())), runOutput;
   if (!runDiffTreeWithRenameDetection(runCmd, &runOutput))
      return nullptr;

   return insertNewFiles(mySha, runOutput);
}

const RevisionFile *Git::getFiles(const QString &sha, const QString &diffToSha, bool allFiles, const QString &path)
{

   const Revision *r = mRevCache->revLookup(sha);
   if (!r)
      return nullptr;

   if (r->parentsCount() == 0) // skip initial Revision
      return nullptr;

   if (r->parentsCount() > 1 && diffToSha.isEmpty() && allFiles)
      return getAllMergeFiles(r);

   if (!diffToSha.isEmpty() && (sha != ZERO_SHA))
   {

      QString runCmd("git diff-tree --no-color -r -m ");
      runCmd.append(diffToSha + " " + sha);
      if (!path.isEmpty())
         runCmd.append(" " + path);

      QString runOutput;
      if (!runDiffTreeWithRenameDetection(runCmd, &runOutput))
         return nullptr;

      // we insert a dummy revision file object. It will be
      // overwritten at each request but we don't care.
      return insertNewFiles(CUSTOM_SHA, runOutput);
   }
   if (mRevsFiles.contains(r->sha()))
      return mRevsFiles[r->sha()]; // ZERO_SHA search arrives here

   if (sha == ZERO_SHA)
      return nullptr;

   QString runCmd("git diff-tree --no-color -r -c " + sha), runOutput;
   if (!runDiffTreeWithRenameDetection(runCmd, &runOutput))
      return nullptr;

   if (mRevsFiles.contains(r->sha())) // has been created in the mean time?
      return mRevsFiles[r->sha()];

   mCacheNeedsUpdate = true;
   return insertNewFiles(sha, runOutput);
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

bool Git::merge(const QString &into, QStringList sources, QString *error)
{
   if (error)
      *error = "";

   if (!run(QString("git checkout -q %1").arg(into)).first)
      return false; // failed to checkout

   QString cmd = QString("git merge -q --no-commit ") + sources.join(" ");

   const auto ret = run(cmd);
   *error = ret.second;

   if (error->contains("Automatic merge failed"))
   {
      // emit signalMergeWithConflicts();
      return false;
   }

   return ret.first;
}

const QStringList Git::sortShaListByIndex(QStringList &shaList)
{

   QStringList orderedShaList;
   for (auto it : shaList)
      appendNamesWithId(orderedShaList, it, QStringList(it), true);

   orderedShaList.sort();
   QStringList::iterator itN(orderedShaList.begin());
   for (; itN != orderedShaList.end(); ++itN) // strip 'idx'
      (*itN) = (*itN).section(' ', -1, -1);

   return orderedShaList;
}

const QStringList Git::getOtherFiles(const QStringList &selFiles)
{

   const RevisionFile *files = getFiles(ZERO_SHA); // files != nullptr
   QStringList notSelFiles;
   for (auto i = 0; i < files->count(); ++i)
   {
      const QString &fp = filePath(*files, i);
      if (selFiles.indexOf(fp) == -1 && files->statusCmp(i, RevisionFile::IN_INDEX))
         notSelFiles.append(fp);
   }
   return notSelFiles;
}

bool Git::updateIndex(const QStringList &selFiles)
{
   const auto files = getFiles(ZERO_SHA); // files != nullptr

   QStringList toAdd, toRemove;

   for (auto it : selFiles)
   {
      const auto idx = findFileIndex(*files, it);

      idx != -1 && files->statusCmp(idx, RevisionFile::DELETED) ? toRemove << it : toAdd << it;
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

bool Git::apply(const QString &fileName, bool asCommit)
{
   QString cmd = asCommit ? QString("git am --signof < ") : QString("git apply ");
   return run(QString("%1 %2").arg(cmd, fileName)).first;
}

bool Git::push(bool force)
{
   QString output;
   const auto ret = run(QString("git push ").append(force ? QString("--force") : QString()));
   output = ret.second;

   if (!ret.first || output.contains("fatal") || output.contains("has no upstream branch"))
      return run(QString("git push --set-upstream origin %1").arg(mCurrentBranchName)).first;

   return ret.first;
}

bool Git::pull(QString &output)
{
   const auto ret = run("git pull");
   output = ret.second;

   return ret.first;
}

bool Git::fetch()
{
   return run("git fetch --all --tags --prune --force").first;
}

bool Git::cherryPickCommit(const QString &sha)
{
   return run(QString("git cherry-pick %1").arg(sha)).first;
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

bool Git::getGitDBDir(const QString &wd, QString &gd, bool &changed)
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
      changed = (d.absolutePath() != mGitDir);
      gd = d.absolutePath();
   }
   return success.first;
}

bool Git::getBaseDir(const QString &wd, QString &bd, bool &changed)
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
   const auto runOutput = ret.second.trimmed();
   if (ret.first)
   {
      // 'git rev-parse --show-cdup' is relative to working directory.
      QDir d(wd + "/" + runOutput);
      bd = d.absolutePath();
      changed = (bd != mWorkingDir);
   }
   else
   {
      changed = true;
      bd = wd;
   }
   return ret.first;
}

Git::Reference *Git::lookupOrAddReference(const QString &sha)
{
   QHash<QString, Reference>::iterator it(mRefsShaMap.find(sha));
   if (it == mRefsShaMap.end())
      it = mRefsShaMap.insert(sha, Reference());
   return &(*it);
}

Git::Reference *Git::lookupReference(const QString &sha)
{
   QHash<QString, Reference>::iterator it(mRefsShaMap.find(sha));
   if (it == mRefsShaMap.end())
      return nullptr;

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
   mShaBackupBuf.clear(); // revs are already empty now

   QString prevRefSha;
   QStringList patchNames, patchShas;
   const QStringList rLst(ret3.second.split('\n', QString::SkipEmptyParts));
   for (auto it : rLst)
   {

      const auto revSha = it.left(40);
      const auto refName = it.mid(41);

      // one Revision could have many tags
      Reference *cur = lookupOrAddReference(toPersistentSha(revSha, mShaBackupBuf));

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
   Reference *cur = lookupOrAddReference(toPersistentSha(curBranchSHA, mShaBackupBuf));
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

Revision *Git::fakeRevData(const QString &sha, const QStringList &parents, const QString &author, const QString &date,
                           const QString &log, const QString &longLog, const QString &patch, int idx)
{

   QString data('>' + sha + 'X' + parents.join(" ") + " \n");
   data.append(author + '\n' + author + '\n' + date + '\n');
   data.append(log + '\n' + longLog);

   QString header("log size " + QString::number(QByteArray(data.toLatin1()).length() - 1) + '\n');
   data.prepend(header);
   if (!patch.isEmpty())
      data.append('\n' + patch);

#if QT_VERSION >= 0x050000
   QTextCodec *tc = QTextCodec::codecForLocale();
   QByteArray *ba = new QByteArray(tc->fromUnicode(data));
#else
   QByteArray *ba = new QByteArray(data.toLatin1());
#endif
   ba->append('\0');

   int dummy;
   Revision *c = new Revision(*ba, 0, idx, &dummy);
   return c;
}

const Revision *Git::fakeWorkDirRev(const QString &parent, const QString &log, const QString &longLog, int idx)
{

   QString patch;
   QString date(QString::number(QDateTime::currentDateTime().toSecsSinceEpoch()));
   QString author("-");
   QStringList parents(parent);
   Revision *c = fakeRevData(ZERO_SHA, parents, author, date, log, longLog, patch, idx);
   c->isDiffCache = true;
   c->lanes.append(LaneType::EMPTY);
   return c;
}

const RevisionFile *Git::fakeWorkDirRevFile(const WorkingDirInfo &wd)
{

   FileNamesLoader fl;
   RevisionFile *rf = new RevisionFile();
   parseDiffFormat(*rf, wd.diffIndex, fl);
   rf->onlyModified = false;

   for (auto it : wd.otherFiles)
   {

      appendFileName(*rf, it, fl);
      rf->status.append(RevisionFile::UNKNOWN);
      rf->mergeParent.append(1);
   }
   RevisionFile cachedFiles;
   parseDiffFormat(cachedFiles, wd.diffIndexCached, fl);
   flushFileNames(fl);

   for (auto i = 0; i < rf->count(); i++)
      if (findFileIndex(cachedFiles, filePath(*rf, i)) != -1)
         rf->status[i] |= RevisionFile::IN_INDEX;
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
   mRevsFiles.insert(ZERO_SHA, fakeWorkDirRevFile(workingDirInfo));

   // then mockup the corresponding Revision
   const QString &log = (isNothingToCommit() ? QString("No local changes") : QString("Local changes"));
   const Revision *r = fakeWorkDirRev(head, log, status, mRevCache->revOrderCount());
   mRevCache->insertRevision(ZERO_SHA, *r);
   mRevData->earlyOutputCntBase = mRevCache->revOrderCount();

   // finally send it to GUI
   emit newRevsAdded();
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
   const QString &type = sl[0];
   const QString &orig = sl[1];
   const QString &dest = sl[2];
   const QString extStatusInfo(orig + " --> " + dest + " (" + type + "%)");

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

bool Git::startParseProc(const QStringList &initCmd)
{
   DataLoader *dl = new DataLoader(this); // auto-deleted when done
   connect(this, &Git::cancelLoading, dl, &DataLoader::on_cancel);
   connect(dl, &DataLoader::newDataReady, this, &Git::newRevsAdded);
   connect(dl, &DataLoader::loaded, this, &Git::on_loaded);

   QString buf;
   return dl->start(initCmd, mWorkingDir, buf);
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

   QStringList initCmd(baseCmd.split(' '));

   return startParseProc(initCmd);
}

void Git::stop(bool saveCache)
{
   // stop all data sending from process and asks them
   // to terminate. Note that process could still keep
   // running for a while although silently
   emit cancelAllProcesses(); // non blocking

   if (mCacheNeedsUpdate && saveCache)
   {

      mCacheNeedsUpdate = false;
      if (!mFilesLoadingCurrentSha.isEmpty()) // we are in the middle of a loading
         mRevsFiles.remove(mFilesLoadingCurrentSha); // remove partial data

      if (!mRevsFiles.isEmpty())
         saveOnCache(mGitDir, mRevsFiles, mDirNames, mFileNames);
   }
}

void Git::clearRevs()
{
   mRevData->clear();
   mFirstNonStGitPatch = "";
   workingDirInfo.clear();
   mRevsFiles.remove(ZERO_SHA);
}

void Git::clearFileNames()
{

   qDeleteAll(mRevsFiles);
   mRevsFiles.clear();
   mFileNamesMap.clear();
   mDirNamesMap.clear();
   mDirNames.clear();
   mFileNames.clear();
   mRevsFilesShaBackupBuf.clear();
   mCacheNeedsUpdate = false;
}

bool Git::init(const QString &wd, QSharedPointer<RevisionsCache> revCache)
{
   QLog_Info("Git", "Initializing Git...");

   mRevCache = revCache;

   // normally called when changing git directory. Must be called after stop()
   clearRevs();

   const QString msg1("Path is '" + mWorkingDir + "'    Loading ");

   // check if repository is valid
   bool repoChanged;
   const auto isGIT = getGitDBDir(wd, mGitDir, repoChanged);

   if (repoChanged)
   {
      bool dummy;
      getBaseDir(wd, mWorkingDir, dummy);
      clearFileNames();
      mFileCacheAccessed = false;

      loadFileCache();
   }

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

void Git::on_loaded(ulong byteSize, int loadTime, bool normalExit)
{
   if (normalExit)
   { // do not send anything if killed

      emit newRevsAdded();

      mRevData->loadTime += loadTime;

      ulong kb = byteSize / 1024;
      double mbs = static_cast<double>(byteSize) / mRevData->loadTime / 1000;
      QString tmp;
      tmp.sprintf("Loaded %i revisions  (%li KB),   "
                  "time elapsed: %i ms  (%.2f MB/s)",
                  mRevCache->count(), kb, mRevData->loadTime, mbs);

      emit loadCompleted(tmp);
   }
}

bool Git::saveOnCache(const QString &gitDir, const QHash<QString, const RevisionFile *> &rf,
                      const QVector<QString> &dirs, const QVector<QString> &files)
{
   if (gitDir.isEmpty() || rf.isEmpty())
      return false;

   const auto path = QString("%1/%2").arg(gitDir, kCacheFileName);
   const auto tmpPath = QString("%1.bak").arg(path);

   QDir dir;
   if (!dir.exists(gitDir))
   {
      return false;
   }
   QFile f(tmpPath);
   if (!f.open(QIODevice::WriteOnly | QIODevice::Unbuffered))
      return false;

   // compress in memory before write to file
   QByteArray data;
   QDataStream stream(&data, QIODevice::WriteOnly);

   // Write a header with a "magic number" and a version
   stream << static_cast<quint32>(C_MAGIC);
   stream << static_cast<qint32>(C_VERSION);

   stream << static_cast<qint32>(dirs.count());
   for (int i = 0; i < dirs.count(); ++i)
      stream << dirs.at(i);

   stream << static_cast<qint32>(files.count());
   for (int i = 0; i < files.count(); ++i)
      stream << files.at(i);

   // to achieve a better compression we save the sha's as
   // one very long string instead of feeding the stream with
   // each one. With this trick we gain a 15% size reduction
   // in the final compressed file. The save/load speed is
   // almost the same.
   int bufSize = rf.count() * 41 + 1000; // a little bit more space then required

   QByteArray buf;
   buf.reserve(bufSize);

   QVector<const RevisionFile *> v;
   v.reserve(rf.count());

   QVector<QByteArray> ba;
   QString CUSTOM_SHA_RAW(toPersistentSha(CUSTOM_SHA, ba));
   int newSize = 0;

   const auto rfEnd = rf.cend();
   for (auto it = rf.begin(); it != rfEnd; ++it)
   {
      const QString &sha = it.key();
      if (sha == ZERO_SHA || sha == CUSTOM_SHA_RAW || sha[0] == 'A') // ALL_MERGE_FILES + Revision sha
         continue;

      v.append(it.value());
      buf.append(sha.toUtf8()).append('\0');
      newSize += 41;
      if (newSize > bufSize)
      {
         return false;
      }
   }
   buf.resize(newSize);
   stream << static_cast<qint32>(newSize);
   stream << buf;

   for (int i = 0; i < v.size(); ++i)
      *(v.at(i)) >> stream;

   f.write(qCompress(data, 1)); // no need to encode with compressed data
   f.close();

   if (dir.exists(path))
   {
      if (!dir.remove(path))
      {
         dir.remove(tmpPath);
         return false;
      }
   }
   dir.rename(tmpPath, path);
   return true;
}

bool Git::loadFromCache(const QString &gitDir, QHash<QString, const RevisionFile *> &rfm, QVector<QString> &dirs,
                        QVector<QString> &files, QByteArray &revsFilesShaBuf)
{
   // check for cache file
   QString path = QString("%1/%2").arg(gitDir, kCacheFileName);
   QFile f(path);
   if (!f.exists())
      return true; // no cache file is not an error

   if (!f.open(QIODevice::ReadOnly | QIODevice::Unbuffered))
      return false;

   QDataStream stream(qUncompress(f.readAll()));
   quint32 magic;
   qint32 version;
   qint32 dirsNum, filesNum, bufSize;
   stream >> magic;
   stream >> version;
   if (magic != C_MAGIC || version != C_VERSION)
   {
      f.close();
      return false;
   }
   // read the data
   stream >> dirsNum;
   dirs.resize(dirsNum);
   for (int i = 0; i < dirsNum; ++i)
      stream >> dirs[i];

   stream >> filesNum;
   files.resize(filesNum);
   for (int i = 0; i < filesNum; ++i)
      stream >> files[i];

   stream >> bufSize;
   revsFilesShaBuf.clear();
   revsFilesShaBuf.reserve(bufSize);
   stream >> revsFilesShaBuf;

   const char *data = revsFilesShaBuf.constData();

   while (!stream.atEnd())
   {

      RevisionFile *rf = new RevisionFile();
      *rf << stream;

      QString sha = QString::fromUtf8(data);
      rfm.insert(sha, rf);

      data += 40;
      if (*data != '\0')
         return false;
      data++;
   }
   f.close();
   return true;
}

bool Git::populateRenamedPatches(const QString &renamedSha, const QStringList &newNames, QStringList *oldNames,
                                 bool backTrack)
{

   const auto ret = run("git diff-tree -r -M " + renamedSha);
   if (!ret.first)
      return false;

   QString runOutput = ret.second;

   // find the first renamed file with the new file name in renamedFiles list
   QString line;
   for (auto it : newNames)
   {
      if (backTrack)
      {
         line = runOutput.section('\t' + it + '\t', 0, 0, QString::SectionIncludeTrailingSep);
         line.chop(1);
      }
      else
         line = runOutput.section('\t' + it + '\n', 0, 0);

      if (!line.isEmpty())
         break;
   }
   if (line.contains('\n'))
      line = line.section('\n', -1, -1);

   const QString &status = line.section('\t', -2, -2).section(' ', -1, -1);
   if (!status.startsWith('R'))
      return false;

   if (backTrack)
   {
      const QString &nextFile = runOutput.section(line, 1, 1).section('\t', 1, 1);
      oldNames->append(nextFile.section('\n', 0, 0));
      return true;
   }
   // get the diff betwen two files
   const QString &prevFileSha = line.section(' ', 2, 2);
   const QString &lastFileSha = line.section(' ', 3, 3);
   if (prevFileSha == lastFileSha) // just renamed
      runOutput.clear();
   else
   {
      const auto ret2 = run("git diff --no-ext-diff -r --full-index " + prevFileSha + " " + lastFileSha);
      if (!ret2.first)
         return false;

      runOutput = ret2.second;
   }

   const QString &prevFile = line.section('\t', -1, -1);
   if (!oldNames->contains(prevFile))
      oldNames->append(prevFile);

   // save the patch, will be used later to create a
   // proper graft sha with correct parent info
   if (mRevData)
   {
      QString tmp(!runOutput.isEmpty() ? runOutput : "diff --no-ext-diff --\nsimilarity index 100%\n");
      mRevData->renamedPatches.insert(renamedSha, tmp);
   }
   return true;
}

void Git::populateFileNamesMap()
{

   for (int i = 0; i < mDirNames.count(); ++i)
      mDirNamesMap.insert(mDirNames[i], i);

   for (int i = 0; i < mFileNames.count(); ++i)
      mFileNamesMap.insert(mFileNames[i], i);
}

void Git::loadFileCache()
{
   if (!mFileCacheAccessed)
   {
      mFileCacheAccessed = true;
      QByteArray shaBuf;

      if (loadFromCache(mGitDir, mRevsFiles, mDirNames, mFileNames, shaBuf))
      {
         mRevsFilesShaBackupBuf.append(shaBuf);
         populateFileNamesMap();
      }
   }
}

bool Git::filterEarlyOutputRev(Revision *revision)
{

   if (mRevData->earlyOutputCnt < mRevCache->revOrderCount())
   {

      const QString &sha = mRevCache->createRevisionSha(mRevData->earlyOutputCnt++);
      const auto c = mRevCache->revLookup(sha);
      if (c)
      {
         if (revision->sha() != sha || revision->parents() != c->parents())
         {
            // mismatch found! set correct value, 'Revision' will
            // overwrite 'c' upon returning
            revision->orderIdx = c->orderIdx;
            mRevData->clear(false); // flush the tail
         }
         else
            return true; // filter out 'Revision'
      }
   }
   // we have new revisions, exit from early output state
   mRevData->setEarlyOutputState(false);
   return false;
}

int Git::addChunk(const QByteArray &ba, int start)
{
   int nextStart;
   Revision *revision;

   do
   {
      // only here we create a new Revision
      revision = new Revision(ba, static_cast<uint>(start), mRevCache->revOrderCount(), &nextStart);

      if (nextStart == -2)
      {
         delete revision;
         mRevData->setEarlyOutputState(true);
         start = ba.indexOf('\n', start) + 1;
      }

   } while (nextStart == -2);

   if (nextStart == -1)
   { // half chunk detected
      delete revision;
      return -1;
   }

   const auto sha = revision->sha();

   if (mRevData->earlyOutputCnt != -1 && filterEarlyOutputRev(revision))
   {
      delete revision;
      return nextStart;
   }

   if (!(revision->parentsCount() > 1 && mRevCache->contains(sha)))
   {
      mRevCache->insertRevision(sha, *revision);

      emit mRevCache->signalCacheUpdated();
      emit newRevsAdded();
   }

   return nextStart;
}

bool Git::copyDiffIndex(const QString &parent)
{
   // must be called with empty revs and empty revOrder

   if (mRevCache->revOrderCount() != 0 || !mRevCache->isEmpty())
      return false;

   const Revision *r = mRevCache->revLookup(ZERO_SHA);
   if (!r)
      return false;

   const RevisionFile *files = getFiles(ZERO_SHA);
   if (!files || findFileIndex(*files, mRevData->fileNames().first()) == -1)
      return false;

   // insert a custom ZERO_SHA Revision with proper parent
   const Revision *rf = fakeWorkDirRev(parent, "Working directory changes", "long log\n", 0);
   mRevCache->insertRevision(ZERO_SHA, *rf);

   emit mRevCache->signalCacheUpdated();
   emit newRevsAdded();

   return true;
}

void Git::setLane(const QString &sha)
{

   Lanes *l = mRevData->lns;
   uint i = mRevData->firstFreeLane;
   QVector<QByteArray> ba;
   const QString &ss = toPersistentSha(sha, ba);

   for (uint cnt = static_cast<uint>(mRevCache->revOrderCount()); i < cnt; ++i)
   {

      const QString &curSha = mRevCache->getRevisionSha(static_cast<int>(i));
      Revision *r = const_cast<Revision *>(mRevCache->revLookup(curSha));
      if (r->lanes.count() == 0)
         updateLanes(*r, *l, curSha);

      if (curSha == ss)
         break;
   }
   mRevData->firstFreeLane = ++i;
}

void Git::updateLanes(Revision &c, Lanes &lns, const QString &sha)
{
   // we could get third argument from c.sha(), but we are in fast path here
   // and c.sha() involves a deep copy, so we accept a little redundancy

   if (lns.isEmpty())
      lns.init(sha);

   bool isDiscontinuity;
   bool isFork = lns.isFork(sha, isDiscontinuity);
   bool isMerge = (c.parentsCount() > 1);
   bool isInitial = (c.parentsCount() == 0);

   if (isDiscontinuity)
      lns.changeActiveLane(sha); // uses previous isBoundary state

   lns.setBoundary(c.isBoundary()); // update must be here

   if (isFork)
      lns.setFork(sha);
   if (isMerge)
      lns.setMerge(c.parents());
   if (c.isApplied)
      lns.setApplied();
   if (isInitial)
      lns.setInitial();

   lns.setLanes(c.lanes); // here lanes are snapshotted

   const QString &nextSha = (isInitial) ? "" : QString(c.parent(0));

   lns.nextParent(nextSha);

   if (c.isApplied)
      lns.afterApplied();
   if (isMerge)
      lns.afterMerge();
   if (isFork)
      lns.afterFork();
   if (lns.isBranch())
      lns.afterBranch();

   //	QString tmp = "", tmp2;
   //	for (uint i = 0; i < c.lanes.count(); i++) {
   //		tmp2.setNum(c.lanes[i]);
   //		tmp.append(tmp2 + "-");
   //	}
   //	qDebug("%s %s", tmp.toUtf8().data(), sha.toUtf8().data());
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

void Git::updateDescMap(const Revision *r, uint idx, QHash<QPair<uint, uint>, bool> &dm, QHash<uint, QVector<int>> &dv)
{

   QVector<int> descVec;
   if (r->descRefsMaster != -1)
   {

      const Revision *tmp = mRevCache->revLookup(mRevCache->createRevisionSha(r->descRefsMaster));
      const QVector<int> &nr = tmp->descRefs;

      for (int i = 0; i < nr.count(); i++)
      {

         if (!dv.contains(nr[i]))
            return;

         const QVector<int> &dvv = dv[nr[i]];

         // copy the whole vector instead of each element
         // in the first iteration of the loop below
         descVec = dvv; // quick (shared) copy

         for (int y = 0; y < dvv.count(); y++)
         {

            uint v = (uint)dvv[y];
            QPair<uint, uint> key = qMakePair(idx, v);
            QPair<uint, uint> keyN = qMakePair(v, idx);
            dm.insert(key, true);
            dm.insert(keyN, false);

            // we don't want duplicated entry, otherwise 'dvv' grows
            // greatly in repos with many tagged development branches
            if (i > 0 && !descVec.contains(v)) // i > 0 is rare, no
               descVec.append(v); // need to optimize
         }
      }
   }
   descVec.append(idx);
   dv.insert(idx, descVec);
}

void Git::mergeBranches(Revision *p, const Revision *r)
{

   int r_descBrnMaster = (checkRef(r->sha(), BRANCH | RMT_BRANCH) ? r->orderIdx : r->descBrnMaster);

   if (p->descBrnMaster == r_descBrnMaster || r_descBrnMaster == -1)
      return;

   // we want all the descendant branches, so just avoid duplicates
   const QVector<int> &src1 = mRevCache->revLookup(mRevCache->createRevisionSha(p->descBrnMaster))->descBranches;
   const QVector<int> &src2 = mRevCache->revLookup(mRevCache->createRevisionSha(r_descBrnMaster))->descBranches;
   QVector<int> dst(src1);
   for (int i = 0; i < src2.count(); i++)
      if (std::find(src1.constBegin(), src1.constEnd(), src2[i]) == src1.constEnd())
         dst.append(src2[i]);

   p->descBranches = dst;
   p->descBrnMaster = p->orderIdx;
}

void Git::mergeNearTags(bool down, Revision *p, const Revision *r, const QHash<QPair<uint, uint>, bool> &dm)
{

   bool isTag = checkRef(r->sha(), TAG);
   int r_descRefsMaster = isTag ? r->orderIdx : r->descRefsMaster;
   int r_ancRefsMaster = isTag ? r->orderIdx : r->ancRefsMaster;

   if (down && (p->descRefsMaster == r_descRefsMaster || r_descRefsMaster == -1))
      return;

   if (!down && (p->ancRefsMaster == r_ancRefsMaster || r_ancRefsMaster == -1))
      return;

   // we want the nearest tag only, so remove any tag
   // that is ancestor of any other tag in p U r
   const QString &sha1 = mRevCache->getRevisionSha(down ? p->descRefsMaster : p->ancRefsMaster);
   const QString &sha2 = mRevCache->getRevisionSha(down ? r_descRefsMaster : r_ancRefsMaster);
   const QVector<int> &src1 = down ? mRevCache->revLookup(sha1)->descRefs : mRevCache->revLookup(sha1)->ancRefs;
   const QVector<int> &src2 = down ? mRevCache->revLookup(sha2)->descRefs : mRevCache->revLookup(sha2)->ancRefs;
   QVector<int> dst(src1);

   for (int s2 = 0; s2 < src2.count(); s2++)
   {

      bool add = false;
      for (int s1 = 0; s1 < src1.count(); s1++)
      {

         if (src2[s2] == src1[s1])
         {
            add = false;
            break;
         }
         QPair<uint, uint> key = qMakePair((uint)src2[s2], (uint)src1[s1]);

         if (!dm.contains(key))
         { // could be empty if all tags are independent
            add = true; // could be an independent path
            continue;
         }
         add = (down && dm[key]) || (!down && !dm[key]);
         if (add)
            dst[s1] = -1; // mark for removing
         else
            break;
      }
      if (add)
         dst.append(src2[s2]);
   }
   QVector<int> &nearRefs = (down ? p->descRefs : p->ancRefs);
   int &nearRefsMaster = (down ? p->descRefsMaster : p->ancRefsMaster);

   nearRefs.clear();
   for (int s2 = 0; s2 < dst.count(); s2++)
      if (dst[s2] != -1)
         nearRefs.append(dst[s2]);

   nearRefsMaster = p->orderIdx;
}

void Git::indexTree()
{
   if (mRevCache->revOrderCount() == 0)
      return;

   // we keep the pairs(x, y). Value is true if x is
   // ancestor of y or false if y is ancestor of x
   QHash<QPair<uint, uint>, bool> descMap;
   QHash<uint, QVector<int>> descVect;

   // walk down the tree from latest to oldest,
   // compute children and nearest descendants
   for (int i = 0, cnt = mRevCache->revOrderCount(); i < cnt; ++i)
   {

      const auto shaToLookup = mRevCache->getRevisionSha(i);
      auto type = checkRef(shaToLookup);
      auto isB = type & (BRANCH | RMT_BRANCH);
      auto isT = type & TAG;

      const Revision *r = mRevCache->revLookup(shaToLookup);

      if (isB)
      {
         auto rr = const_cast<Revision *>(r);

         if (r->descBrnMaster != -1)
         {
            const auto sha = mRevCache->getRevisionSha(r->descBrnMaster);
            rr->descBranches = mRevCache->revLookup(sha)->descBranches;
         }

         rr->descBranches.append(i);
      }
      if (isT)
      {
         updateDescMap(r, i, descMap, descVect);
         auto rr = const_cast<Revision *>(r);
         rr->descRefs.clear();
         rr->descRefs.append(i);
      }
      for (uint y = 0; y < r->parentsCount(); y++)
      {
         if (auto p = const_cast<Revision *>(mRevCache->revLookup(r->parent(y))))
         {
            p->children.append(i);

            if (p->descBrnMaster == -1)
               p->descBrnMaster = isB ? r->orderIdx : r->descBrnMaster;
            else
               mergeBranches(p, r);

            if (p->descRefsMaster == -1)
               p->descRefsMaster = isT ? r->orderIdx : r->descRefsMaster;
            else
               mergeNearTags(true, p, r, descMap);
         }
      }
   }
   // walk backward through the tree and compute nearest tagged ancestors
   for (auto i = mRevCache->revOrderCount() - 1; i >= 0; --i)
   {
      const auto r = mRevCache->revLookup(mRevCache->getRevisionSha(i));
      const auto isTag = checkRef(mRevCache->getRevisionSha(i), TAG);

      if (isTag)
      {
         Revision *rr = const_cast<Revision *>(r);
         rr->ancRefs.clear();
         rr->ancRefs.append(i);
      }
      for (int y = 0; y < r->children.count(); y++)
      {
         const auto revSha = mRevCache->getRevisionSha(r->children[y]);
         auto c = const_cast<Revision *>(mRevCache->revLookup(revSha));

         if (c)
         {
            if (c->ancRefsMaster == -1)
               c->ancRefsMaster = isTag ? r->orderIdx : r->ancRefsMaster;
            else
               mergeNearTags(false, c, r, descMap);
         }
      }
   }
}
