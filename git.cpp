/*
        Description: interface to git programs

        Author: Marco Costalba (C) 2005-2007

        Copyright: See COPYING file that comes with this distribution

*/
#include "git.h"
#include "RepositoryModel.h"
#include "dataloader.h"
#include "lanes.h"
#include "myprocess.h"

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

#define GIT_LOG_FORMAT "%m%HX%PX%n%cn<%ce>%n%an<%ae>%n%at%n%s%n"

using namespace QGit;

Git *Git::INSTANCE = nullptr;

Git *Git::getInstance(QObject *p)
{
   if (INSTANCE == nullptr)
      INSTANCE = new Git(p);

   return INSTANCE;
}

Git::Git(QObject *p)
   : QObject(p)
{
   setParent(p);

   mRevsFiles.reserve(MAX_DICT_SIZE);
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

   mErrorReportingEnabled = false; // 'git config' could fail, see docs

   run("git config user.name", &user);
   run("git config user.email", &email);
   info << "Local config" << user << email;

   run("git config --global user.name", &user);
   run("git config --global user.email", &email);
   info << "Global config" << user << email;

   mErrorReportingEnabled = true;
}

QStringList Git::getGitConfigList(bool global)
{
   QString runOutput;

   mErrorReportingEnabled = false; // 'git config' could fail, see docs

   global ? run("git config --global --list", &runOutput) : run("git config --list", &runOutput);

   mErrorReportingEnabled = true;

   return runOutput.split('\n', QString::SkipEmptyParts);
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

   RefMap::const_iterator it(mRefsShaMap.constFind(sha));
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
   mErrorReportingEnabled = false;
   bool ok = run("git rev-parse --revs-only " + refName, &runOutput);
   mErrorReportingEnabled = true;
   return (ok ? runOutput.trimmed() : "");
}

void Git::appendNamesWithId(QStringList &names, const QString &sha, const QStringList &data, bool onlyLoaded)
{

   const Rev *r = revLookup(sha);
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
      QString ro;
      if (run("git cat-file tag " + rf.tagObj, &ro))
         rf.tagMsg = ro.section("\n\n", 1).remove(pgp).trimmed();
   }
   return rf.tagMsg;
}

void Git::addExtraFileInfo(QString *rowName, const QString &sha, const QString &diffToSha, bool allMergeFiles)
{

   const RevFile *files = getFiles(sha, diffToSha, allMergeFiles);
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

void Git::cancelDataLoading(const RepositoryModel *fh)
{
   // normally called when closing file viewer

   emit cancelLoading(fh); // non blocking
}

const Rev *Git::revLookup(const QString &sha, const RepositoryModel *fh) const
{
   const RevMap &r = fh ? fh->revs : mRevData->revs;
   return !sha.isEmpty() ? r.value(sha) : nullptr;
}

bool Git::run(const QString &runCmd, QString *runOutput, QObject *receiver, const QString &buf)
{

   QByteArray ba;
   bool ret = run(&ba, runCmd, receiver, buf);

   if (!runOutput)
   {
      runOutput = new QString();
   }
   *runOutput = QString::fromUtf8(ba);

   return ret;
}

bool Git::run(QByteArray *runOutput, const QString &runCmd, QObject *receiver, const QString &buf)
{

   MyProcess p(parent(), mWorkingDir, mErrorReportingEnabled);
   return p.runSync(runCmd, runOutput, receiver, buf);
}

MyProcess *Git::runAsync(const QString &runCmd, QObject *receiver, const QString &buf)
{

   MyProcess *p = new MyProcess(parent(), mWorkingDir, mErrorReportingEnabled);
   if (!p->runAsync(runCmd, receiver, buf))
   {
      delete p;
      p = nullptr;
   }
   return p; // auto-deleted when done
}

int Git::findFileIndex(const RevFile &rf, const QString &name)
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

   const Rev *rs = revLookup(fromSHA);
   if (!rs)
      return "";

   for (int idx = rs->orderIdx - 1; idx >= 0; idx--)
   {

      const Rev *r = revLookup(mRevData->revOrder[idx]);
      if (laneNum >= r->lanes.count())
         return "";

      if (!isFreeLane(r->lanes[laneNum]))
      {

         int type = r->lanes[laneNum], parNum = 0;
         while (!isMerge(type) && type != ACTIVE)
         {

            if (isHead(type))
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
   const Rev *r = revLookup(parent);
   if (!r)
      return children;

   for (int i = 0; i < r->children.count(); i++)
      children.append(mRevData->revOrder[r->children[i]]);

   // reorder children by loading order
   QStringList::iterator itC(children.begin());
   for (; itC != children.end(); ++itC)
   {
      const Rev *r = revLookup(*itC);
      (*itC).prepend(QString("%1 ").arg(r->orderIdx, 6));
   }
   children.sort();
   for (itC = children.begin(); itC != children.end(); ++itC)
      (*itC) = (*itC).section(' ', -1, -1);

   return children;
}

const QString Git::getShortLog(const QString &sha)
{

   const Rev *r = revLookup(sha);
   return (r ? r->shortLog() : "");
}

MyProcess *Git::getDiff(const QString &sha, QObject *receiver, const QString &diffToSha, bool combined)
{

   if (sha.isEmpty())
      return nullptr;

   QString runCmd;
   if (sha != ZERO_SHA)
   {
      runCmd = "git diff-tree --no-color -r --patch-with-stat ";
      runCmd.append(combined ? QString("-c ") : QString("-C -m ")); // TODO rename for combined

      const Rev *r = revLookup(sha);
      if (r && r->parentsCount() == 0)
         runCmd.append("--root ");

      runCmd.append(diffToSha + " " + sha); // diffToSha could be empty
   }
   else
      runCmd = "git diff-index --no-color -r -m --patch-with-stat HEAD";

   return runAsync(runCmd, receiver);
}

QString Git::getDiff(const QString &currentSha, const QString &previousSha, const QString &file)
{
   QByteArray output;
   const auto ret = run(&output, QString("git diff -U15000 %1 %2 %3").arg(previousSha, currentSha, file));

   if (ret)
      return QString::fromUtf8(output);

   return QString();
}

const QString Git::getWorkDirDiff(const QString &fileName)
{

   QString runCmd("git diff-index --no-color -r -z -m -p --full-index --no-commit-id HEAD"), runOutput;
   if (!fileName.isEmpty())
      runCmd.append(" -- " + quote(fileName));

   if (!run(runCmd, &runOutput))
      return "";

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

   const RevFile *rf = mRevsFiles[ZERO_SHA];
   return rf->count() == workingDirInfo.otherFiles.count();
}

bool Git::resetFile(const QString &fileName)
{
   if (fileName.isEmpty())
      return false;

   QByteArray output;
   QString cmd = QString("git checkout %1").arg(fileName);
   return run(&output, cmd);
}

QPair<QString, QString> Git::getSplitCommitMsg(const QString &sha)
{
   const Rev *c = revLookup(sha);

   if (!c)
      return qMakePair(QString(), QString());

   return qMakePair(c->shortLog(), c->longLog().trimmed());
}

QString Git::getCommitMsg(const QString &sha) const
{
   const Rev *c = revLookup(sha);
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
   QString top;
   if (run("git rev-parse --verify HEAD", &top))
      sha = top.trimmed();
   else
      return "";

   const Rev *c = revLookup(sha);

   if (!c)
      return "";

   return c->shortLog() + "\n\n" + c->longLog().trimmed();
}

const QString Git::getNewCommitMsg()
{

   const Rev *c = revLookup(ZERO_SHA);
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
   QByteArray output;
   QVector<QString> submodulesList;
   if (run(&output, "git config --file .gitmodules --name-only --get-regexp path"))
   {
      const auto submodules = output.split('\n');
      for (auto submodule : submodules)
         if (!submodule.isEmpty() && submodule != "\n")
            submodulesList.append(QString::fromUtf8(submodule.split('.').at(1)));
   }

   return submodulesList;
}

bool Git::submoduleUpdate(const QString &submodule) {}

bool Git::submoduleRemove(const QString &submodule) {}

const RevFile *Git::insertNewFiles(const QString &sha, const QString &data)
{

   /* we use an independent FileNamesLoader to avoid data
    * corruption if we are loading file names in background
    */
   FileNamesLoader fl;

   RevFile *rf = new RevFile();
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

   mErrorReportingEnabled = false;
   bool renameDetectionOk = run(cmd, runOutput);
   mErrorReportingEnabled = true;

   if (!renameDetectionOk) // retry without rename detection
      return run(runCmd, runOutput);

   return true;
}

const RevFile *Git::getAllMergeFiles(const Rev *r)
{

   const QString &mySha(ALL_MERGE_FILES + QString(r->sha()));
   if (mRevsFiles.contains(mySha))
      return mRevsFiles[mySha];

   QString runCmd(QString("git diff-tree --no-color -r -m ").arg(r->sha())), runOutput;
   if (!runDiffTreeWithRenameDetection(runCmd, &runOutput))
      return nullptr;

   return insertNewFiles(mySha, runOutput);
}

const RevFile *Git::getFiles(const QString &sha, const QString &diffToSha, bool allFiles, const QString &path)
{

   const Rev *r = revLookup(sha);
   if (!r)
      return nullptr;

   if (r->parentsCount() == 0) // skip initial rev
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

const QString Git::getNewestFileName(QStringList &branches, const QString &fileName)
{

   QString curFileName(fileName), runOutput, args;
   while (true)
   {
      args = branches.join(" ") + " -- " + curFileName;
      if (!run("git ls-tree " + args, &runOutput))
         break;

      if (!runOutput.isEmpty())
         break;

      QString msg("Retrieving file renames, now at '" + curFileName + "'...");

      if (!run("git rev-list -n1 " + args, &runOutput))
         break;

      if (runOutput.isEmpty()) // try harder
         if (!run("git rev-list --full-history -n1 " + args, &runOutput))
            break;

      if (runOutput.isEmpty())
         break;

      const QString &sha = runOutput.trimmed();
      QStringList newCur;
      if (!populateRenamedPatches(sha, QStringList(curFileName), nullptr, &newCur, true))
         break;

      curFileName = newCur.first();
   }
   return curFileName;
}

bool Git::resetCommits(int parentDepth)
{

   QString runCmd("git reset --soft HEAD~");
   runCmd.append(QString::number(parentDepth));
   return run(runCmd);
}

bool Git::merge(const QString &into, QStringList sources, QString *error)
{
   if (error)
      *error = "";
   if (!run(QString("git checkout -q %1").arg(into)))
      return false; // failed to checkout

   QString cmd = QString("git merge -q --no-commit ") + sources.join(" ");

   const auto ret = run(cmd, error);

   if (error->contains("Automatic merge failed"))
   {
      // emit signalMergeWithConflicts();
      return false;
   }

   return ret;
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

   const RevFile *files = getFiles(ZERO_SHA); // files != nullptr
   QStringList notSelFiles;
   for (auto i = 0; i < files->count(); ++i)
   {
      const QString &fp = filePath(*files, static_cast<unsigned int>(i));
      if (selFiles.indexOf(fp) == -1 && files->statusCmp(i, RevFile::IN_INDEX))
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

      idx != -1 && files->statusCmp(idx, RevFile::DELETED) ? toRemove << it : toAdd << it;
   }
   if (!toRemove.isEmpty() && !run("git rm --cached --ignore-unmatch -- " + quote(toRemove)))
      return false;

   if (!toAdd.isEmpty() && !run("git add -- " + quote(toAdd)))
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
   if ((!notSel.empty() && !run("git reset -- " + quote(notSel))) || !updateIndex(selFiles)
       || !run("git commit" + cmtOptions + " -F " + quote(msgFile)) || (!notSel.empty() && !updateIndex(notSel)))
   {
      QDir dir(mWorkingDir);
      dir.remove(msgFile);
      ret = false;
   }

   return ret;
}

bool Git::push(bool force)
{
   QString output;
   const auto ret = run(QString("git push ").append(force ? QString("--force") : QString()), &output);

   if (!ret || output.contains("fatal") || output.contains("has no upstream branch"))
      return run(QString("git push --set-upstream origin %1").arg(mCurrentBranchName));

   return ret;
}

bool Git::pull(QString &output)
{
   return run("git pull", &output);
}

bool Git::fetch()
{
   return run("git fetch --all");
}

bool Git::cherryPickCommit(const QString &sha)
{
   return run(QString("git cherry-pick %1").arg(sha));
}

bool Git::pop()
{
   /*
   const QStringList patch(getRefNames(sha, APPLIED));
   if (patch.count() != 1)
   {
      dbp("ASSERT in Git::stgPop, found %1 patches instead of 1", patch.count());
      return false;
   }
   */
   return run("git pop");
}

bool Git::stash()
{
   return run("git stash");
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

   return run(QString("git reset --%1 %2").arg(typeStr, sha));
}

bool Git::createBranchFromCurrent(const QString &newName, QByteArray &output)
{
   return run(&output, QString("git branch %1").arg(newName), nullptr, "");
}

bool Git::createBranchFromAnotherBranch(const QString &oldName, const QString &newName, QByteArray &output)
{
   return run(&output, QString("git branch %1 %2").arg(newName, oldName), nullptr, "");
}

bool Git::createBranchAtCommit(const QString &commitSha, const QString &branchName, QByteArray &output)
{
   return run(&output, QString("git branch %1 %2").arg(branchName, commitSha), nullptr, "");
}

bool Git::checkoutNewLocalBranch(const QString &branchName, QByteArray &output)
{
   return run(&output, QString("git checkout -b %1").arg(branchName), nullptr, "");
}

bool Git::renameBranch(const QString &oldName, const QString &newName, QByteArray &output)
{
   return run(&output, QString("git branch -m %1 %2").arg(oldName, newName), nullptr, "");
}

bool Git::removeLocalBranch(const QString &branchName, QByteArray &output)
{
   return run(&output, QString("git branch -D %1").arg(branchName), nullptr, "");
}

bool Git::removeRemoteBranch(const QString &branchName, QByteArray &output)
{
   return run(&output, QString("git push --delete origin %1").arg(branchName));
}

bool Git::getBranches(QByteArray &output)
{
   return run(&output, "git branch -a", nullptr, "");
}

bool Git::getDistanceBetweenBranches(bool toMaster, const QString &right, QByteArray &output)
{
   const QString firstArg = toMaster ? QString::fromUtf8("--left-right") : QString::fromUtf8("");
   const QString gitCmd = QString("git rev-list %1 --count %2...%3")
                              .arg(firstArg)
                              .arg(toMaster ? QString::fromUtf8("origin/master") : QString::fromUtf8("origin/%3"))
                              .arg(right);
   return run(&output, gitCmd, nullptr);
}

bool Git::getBranchesOfCommit(const QString &sha, QByteArray &output)
{
   return run(&output, QString("git branch --contains %1 --all").arg(sha));
}

bool Git::getLastCommitOfBranch(const QString &branch, QByteArray &sha)
{
   const auto ret = run(&sha, QString("git rev-parse %1").arg(branch));

   if (ret)
      sha.remove(sha.count() - 1, sha.count());

   return ret;
}

bool Git::prune(QByteArray &output)
{
   return run(&output, "git remote prune origin");
}

QVector<QString> Git::getTags(QByteArray &output)
{
   const auto ret = run(&output, "git tag");

   QVector<QString> tags;

   if (ret)
   {
      const auto tagsTmp = QString::fromUtf8(output).split("\n");

      for (auto tag : tagsTmp)
         if (tag != "\n" && !tag.isEmpty())
            tags.append(tag);
   }

   return tags;
}

bool Git::addTag(const QString &tagName, const QString &tagMessage, const QString &sha, QByteArray &output)
{
   return run(&output, QString("git tag -a %1 %2 -m \"%3\"").arg(tagName).arg(sha).arg(tagMessage));
}

bool Git::removeTag(const QString &tagName, bool remote, QByteArray &output)
{
   auto ret = false;

   if (remote)
      ret = run(&output, QString("git push origin --delete %1").arg(tagName));

   if (!remote || (remote && ret))
      ret = run(&output, QString("git tag -d %1").arg(tagName));

   return ret;
}

bool Git::pushTag(const QString &tagName, QByteArray &output)
{
   return run(&output, QString("git push origin %1").arg(tagName));
}

bool Git::getTagCommit(const QString &tagName, QByteArray &output)
{
   const auto ret = run(&output, QString("git rev-list -n 1 %1").arg(tagName));

   if (ret)
      output.remove(output.count() - 2, output.count() - 1);

   return ret;
}

QVector<QString> Git::getStashes(QByteArray &output)
{
   const auto ret = run(&output, "git stash list");

   QVector<QString> stashes;

   if (ret)
   {
      const auto tagsTmp = QString::fromUtf8(output).split("\n");

      for (auto tag : tagsTmp)
         if (tag != "\n" && !tag.isEmpty())
            stashes.append(tag);
   }

   return stashes;
}

bool Git::getStashCommit(const QString &stash, QByteArray &output)
{
   const auto ret = run(&output, QString("git rev-list %1 -n 1 ").arg(stash));

   if (ret)
      output.remove(output.count() - 2, output.count() - 1);

   return ret;
}

//! cache for dates conversion. Common among qgit windows
static QHash<QString, QString> localDates;
/**
 * Accesses a cache that avoids slow date calculation
 *
 * @param gitDate
 *   the reference from which we want to get the date
 *
 * @return
 *   human-readable date
 **/
const QString Git::getLocalDate(const QString &gitDate)
{
   QString localDate(localDates.value(gitDate));

   // cache miss
   if (localDate.isEmpty())
   {
      static QDateTime d;
      d.setSecsSinceEpoch(gitDate.toUInt());
      localDate = d.toString(Qt::SystemLocaleShortDate);

      // save to cache
      localDates[gitDate] = localDate;
   }

   return localDate;
}

bool Git::getGitDBDir(const QString &wd, QString &gd, bool &changed)
{
   // we could run from a subdirectory, so we need to get correct directories

   QString runOutput, tmp(mWorkingDir);
   mWorkingDir = wd;
   mErrorReportingEnabled = false;
   bool success = run("git rev-parse --git-dir", &runOutput); // run under newWorkDir
   mErrorReportingEnabled = true;
   mWorkingDir = tmp;
   runOutput = runOutput.trimmed();
   if (success)
   {
      // 'git rev-parse --git-dir' output could be a relative
      // to working directory (as ex .git) or an absolute path
      QDir d(runOutput.startsWith("/") ? runOutput : wd + "/" + runOutput);
      changed = (d.absolutePath() != mGitDir);
      gd = d.absolutePath();
   }
   return success;
}

bool Git::getBaseDir(const QString &wd, QString &bd, bool &changed)
{
   // we could run from a subdirectory, so we need to get correct directories

   // We use --show-cdup and not --git-dir for this, in order to take into account configurations
   //  in which .git is indeed a "symlink", a text file containing the path of the actual .git database dir.
   // In that particular case, the parent directory of the one given by --git-dir is *not* necessarily
   //  the base directory of the repository.

   QString runOutput, tmp(mWorkingDir);
   mWorkingDir = wd;
   mErrorReportingEnabled = false;
   bool success = run("git rev-parse --show-cdup", &runOutput); // run under newWorkDir
   mErrorReportingEnabled = true;
   mWorkingDir = tmp;
   runOutput = runOutput.trimmed();
   if (success)
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
   return success;
}

Git::Reference *Git::lookupOrAddReference(const QString &sha)
{
   RefMap::iterator it(mRefsShaMap.find(sha));
   if (it == mRefsShaMap.end())
      it = mRefsShaMap.insert(sha, Reference());
   return &(*it);
}

Git::Reference *Git::lookupReference(const QString &sha)
{
   RefMap::iterator it(mRefsShaMap.find(sha));
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
   QString curBranchSHA;
   if (!run("git rev-parse --revs-only HEAD", &curBranchSHA))
      return false;

   if (!run("git branch", &mCurrentBranchName))
      return false;

   curBranchSHA = curBranchSHA.trimmed();
   mCurrentBranchName = mCurrentBranchName.prepend('\n').section("\n*", 1);
   mCurrentBranchName = mCurrentBranchName.section('\n', 0, 0).trimmed();
   if (mCurrentBranchName.contains(" detached "))
      mCurrentBranchName = "";

   // read refs, normally unsorted
   QString runOutput;
   if (!run("git show-ref -d", &runOutput))
      return false;

   mRefsShaMap.clear();
   mShaBackupBuf.clear(); // revs are already empty now

   QString prevRefSha;
   QStringList patchNames, patchShas;
   const QStringList rLst(runOutput.split('\n', QString::SkipEmptyParts));
   for (auto it : rLst)
   {

      const auto revSha = it.left(40);
      const auto refName = it.mid(41);

      // one rev could have many tags
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

   QString runOutput;
   run(runCmd, &runOutput);
   return runOutput.split('\n', QString::SkipEmptyParts);
}

Rev *Git::fakeRevData(const QString &sha, const QStringList &parents, const QString &author, const QString &date,
                      const QString &log, const QString &longLog, const QString &patch, int idx, RepositoryModel *fh)
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

   fh->rowData.append(ba);
   int dummy;
   Rev *c = new Rev(*ba, 0, idx, &dummy, !isMainHistory(fh));
   return c;
}

const Rev *Git::fakeWorkDirRev(const QString &parent, const QString &log, const QString &longLog, int idx,
                               RepositoryModel *fh)
{

   QString patch;
   if (!isMainHistory(fh))
      patch = getWorkDirDiff(fh->fileNames().first());

   QString date(QString::number(QDateTime::currentDateTime().toSecsSinceEpoch()));
   QString author("-");
   QStringList parents(parent);
   Rev *c = fakeRevData(ZERO_SHA, parents, author, date, log, longLog, patch, idx, fh);
   c->isDiffCache = true;
   c->lanes.append(EMPTY);
   return c;
}

const RevFile *Git::fakeWorkDirRevFile(const WorkingDirInfo &wd)
{

   FileNamesLoader fl;
   RevFile *rf = new RevFile();
   parseDiffFormat(*rf, wd.diffIndex, fl);
   rf->onlyModified = false;

   for (auto it : wd.otherFiles)
   {

      appendFileName(*rf, it, fl);
      rf->status.append(RevFile::UNKNOWN);
      rf->mergeParent.append(1);
   }
   RevFile cachedFiles;
   parseDiffFormat(cachedFiles, wd.diffIndexCached, fl);
   flushFileNames(fl);

   for (auto i = 0; i < rf->count(); i++)
      if (findFileIndex(cachedFiles, filePath(*rf, static_cast<unsigned int>(i))) != -1)
         rf->status[i] |= RevFile::IN_INDEX;
   return rf;
}

void Git::getDiffIndex()
{

   QString status;
   if (!run("git status", &status)) // git status refreshes the index, run as first
      return;

   QString head;
   if (!run("git rev-parse --revs-only HEAD", &head))
      return;

   head = head.trimmed();
   if (!head.isEmpty())
   { // repository initialized but still no history

      if (!run("git diff-index " + head, &workingDirInfo.diffIndex))
         return;

      // check for files already updated in cache, we will
      // save this information in status third field
      if (!run("git diff-index --cached " + head, &workingDirInfo.diffIndexCached))
         return;
   }
   // get any file not in tree
   workingDirInfo.otherFiles = getOthersFiles();

   // now mockup a RevFile
   mRevsFiles.insert(ZERO_SHA, fakeWorkDirRevFile(workingDirInfo));

   // then mockup the corresponding Rev
   const QString &log = (isNothingToCommit() ? QString("No local changes") : QString("Local changes"));
   const Rev *r = fakeWorkDirRev(head, log, status, mRevData->revOrder.count(), mRevData);
   mRevData->revs.insert(ZERO_SHA, r);
   mRevData->revOrder.append(ZERO_SHA);
   mRevData->earlyOutputCntBase = mRevData->revOrder.count();

   // finally send it to GUI
   emit newRevsAdded(mRevData, mRevData->revOrder);
}

void Git::parseDiffFormatLine(RevFile &rf, const QString &line, int parNum, FileNamesLoader &fl)
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

// CT TODO can go in RevFile
void Git::setStatus(RevFile &rf, const QString &rowSt)
{

   char status = rowSt.at(0).toLatin1();
   switch (status)
   {
      case 'M':
      case 'T':
      case 'U':
         rf.status.append(RevFile::MODIFIED);
         break;
      case 'D':
         rf.status.append(RevFile::DELETED);
         rf.onlyModified = false;
         break;
      case 'A':
         rf.status.append(RevFile::NEW);
         rf.onlyModified = false;
         break;
      case '?':
         rf.status.append(RevFile::UNKNOWN);
         rf.onlyModified = false;
         break;
      default:
         rf.status.append(RevFile::MODIFIED);
         break;
   }
}

void Git::setExtStatus(RevFile &rf, const QString &rowSt, int parNum, FileNamesLoader &fl)
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
   rf.status.append(RevFile::NEW);
   rf.extStatus.resize(rf.status.size());
   rf.extStatus[rf.status.size() - 1] = extStatusInfo;

   // simulate deleted orig file only in case of rename
   if (type.at(0) == 'R')
   { // renamed file
      appendFileName(rf, orig, fl);
      rf.mergeParent.append(parNum);
      rf.status.append(RevFile::DELETED);
      rf.extStatus.resize(rf.status.size());
      rf.extStatus[rf.status.size() - 1] = extStatusInfo;
   }
   rf.onlyModified = false;
}

// CT TODO utility function; can go elsewhere
void Git::parseDiffFormat(RevFile &rf, const QString &buf, FileNamesLoader &fl)
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

bool Git::startParseProc(const QStringList &initCmd, RepositoryModel *fh, const QString &buf)
{
   DataLoader *dl = new DataLoader(fh); // auto-deleted when done
   connect(this, &Git::cancelLoading, dl, qOverload<const RepositoryModel *>(&DataLoader::on_cancel));
   connect(dl, &DataLoader::newDataReady, this, &Git::on_newDataReady);
   connect(dl, &DataLoader::loaded, this, &Git::on_loaded);

   return dl->start(initCmd, mWorkingDir, buf);
}

bool Git::startRevList(QStringList &args, RepositoryModel *fh)
{

   QString baseCmd("git log --date-order --no-color "

#ifndef Q_OS_WIN32
                   "--log-size " // FIXME broken on Windows
#endif
                   "--parents --boundary -z "
                   "--pretty=format:" GIT_LOG_FORMAT);

   // we don't need log message body for file history
   if (isMainHistory(fh))
      baseCmd.append("%b --all");

   QStringList initCmd(baseCmd.split(' '));
   if (!isMainHistory(fh))
   {
      /*
     NOTE: we don't use '--remove-empty' option because
     in case a file is deleted and then a new file with
     the same name is created again in the same directory
     then, with this option, file history is truncated to
     the file deletion revision.
  */
      initCmd << QString("-r -m -p --full-index").split(' ');
   }
   else
   {
   } // initCmd << QString("--early-output"); currently disabled

   return startParseProc(initCmd + args, fh, QString());
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
   mPatchesStillToFind = 0; // TODO TEST WITH FILTERING
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

bool Git::init(const QString &wd, const QStringList *passedArgs)
{
   // normally called when changing git directory. Must be called after stop()
   clearRevs();

   const QString msg1("Path is '" + mWorkingDir + "'    Loading ");

   // check if repository is valid
   bool repoChanged;
   mIsGIT = getGitDBDir(wd, mGitDir, repoChanged);

   if (repoChanged)
   {
      bool dummy;
      getBaseDir(wd, mWorkingDir, dummy);
      localDates.clear();
      clearFileNames();
      mFileCacheAccessed = false;

      loadFileCache();
   }

   if (!mIsGIT)
      return false;

   if (!passedArgs)
      getRefs(); // load references

   // init2();

   return true;
}

void Git::init2()
{
   getDiffIndex(); // blocking, we could be in setRepository() now

   // build up command line arguments
   QStringList args;
   startRevList(args, mRevData);
}

void Git::on_newDataReady(const RepositoryModel *fh)
{

   emit newRevsAdded(fh, fh->revOrder);
}

void Git::on_loaded(RepositoryModel *fh, ulong byteSize, int loadTime, bool normalExit, const QString &cmd,
                    const QString &errorDesc)
{

   if (!errorDesc.isEmpty())
   {
      MainExecErrorEvent *e = new MainExecErrorEvent(cmd, errorDesc);
      QApplication::postEvent(parent(), e);
   }
   if (normalExit)
   { // do not send anything if killed

      on_newDataReady(fh);

      if (!mLoadingUnappliedPatches)
      {

         fh->loadTime += loadTime;

         ulong kb = byteSize / 1024;
         double mbs = static_cast<double>(byteSize) / fh->loadTime / 1000;
         QString tmp;
         tmp.sprintf("Loaded %i revisions  (%li KB),   "
                     "time elapsed: %i ms  (%.2f MB/s)",
                     fh->revs.count(), kb, fh->loadTime, mbs);

         if (!tryFollowRenames(fh))
            emit loadCompleted(fh, tmp);

         if (isMainHistory(fh))
            // wait the dust to settle down before to start
            // background file names loading for new revisions
            QTimer::singleShot(500, this, &Git::loadFileNames);
      }
   }
   if (mLoadingUnappliedPatches)
   {
      mLoadingUnappliedPatches = false;
      mRevData->lns->clear(); // again to reset lanes
      init2(); // continue with loading of remaining revisions
   }
}

bool Git::saveOnCache(const QString &gitDir, const RevFileMap &rf, const QVector<QString> &dirs,
                      const QVector<QString> &files)
{
   if (gitDir.isEmpty() || rf.isEmpty())
      return false;

   QString path(gitDir + C_DAT_FILE);
   QString tmpPath(path + BAK_EXT);

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
   stream << (quint32)C_MAGIC;
   stream << (qint32)C_VERSION;

   stream << (qint32)dirs.count();
   for (int i = 0; i < dirs.count(); ++i)
      stream << dirs.at(i);

   stream << (qint32)files.count();
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

   QVector<const RevFile *> v;
   v.reserve(rf.count());

   QVector<QByteArray> ba;
   QString CUSTOM_SHA_RAW(toPersistentSha(CUSTOM_SHA, ba));
   int newSize = 0;

   const auto rfEnd = rf.cend();
   for (auto it = rf.begin(); it != rfEnd; ++it)
   {
      const QString &sha = it.key();
      if (sha == ZERO_SHA || sha == CUSTOM_SHA_RAW || sha[0] == 'A') // ALL_MERGE_FILES + rev sha
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
   stream << (qint32)newSize;
   stream << buf;

   for (int i = 0; i < v.size(); ++i)
      *(v.at(i)) >> stream;

   f.write(qCompress(data, 1)); // no need to encode with compressed data
   f.close();

   // rename C_DAT_FILE + BAK_EXT -> C_DAT_FILE
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

bool Git::loadFromCache(const QString &gitDir, RevFileMap &rfm, QVector<QString> &dirs, QVector<QString> &files,
                        QByteArray &revsFilesShaBuf)
{
   // check for cache file
   QString path(gitDir + C_DAT_FILE);
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

      RevFile *rf = new RevFile();
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

bool Git::tryFollowRenames(RepositoryModel *fh)
{

   if (isMainHistory(fh))
      return false;

   QStringList oldNames;
   QMutableStringListIterator it(fh->renamedRevs);
   while (it.hasNext())
      if (!populateRenamedPatches(it.next(), fh->curFNames, fh, &oldNames, false))
         it.remove();

   if (fh->renamedRevs.isEmpty())
      return false;

   QStringList args;
   args << fh->renamedRevs << "--" << oldNames;
   fh->fNames << oldNames;
   fh->curFNames = oldNames;
   fh->renamedRevs.clear();
   return startRevList(args, fh);
}

bool Git::populateRenamedPatches(const QString &renamedSha, const QStringList &newNames, RepositoryModel *fh,
                                 QStringList *oldNames, bool backTrack)
{

   QString runOutput;
   if (!run("git diff-tree -r -M " + renamedSha, &runOutput))
      return false;

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
   else if (!run("git diff --no-ext-diff -r --full-index " + prevFileSha + " " + lastFileSha, &runOutput))
      return false;

   const QString &prevFile = line.section('\t', -1, -1);
   if (!oldNames->contains(prevFile))
      oldNames->append(prevFile);

   // save the patch, will be used later to create a
   // proper graft sha with correct parent info
   if (fh)
   {
      QString tmp(!runOutput.isEmpty() ? runOutput : "diff --no-ext-diff --\nsimilarity index 100%\n");
      fh->renamedPatches.insert(renamedSha, tmp);
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

void Git::loadFileNames()
{

   indexTree(); // we are sure data loading is finished at this point

   int revCnt = 0;
   QString diffTreeBuf;
   for (auto it : mRevData->revOrder)
   {

      if (!mRevsFiles.contains(it))
      {
         const Rev *c = revLookup(it);
         if (c->parentsCount() == 1)
         { // skip initials and merges
            diffTreeBuf.append(it).append('\n');
            revCnt++;
         }
      }
   }
   if (!diffTreeBuf.isEmpty())
   {
      mFilesLoadingPending = mFilesLoadingCurrentSha = "";
      mFilesLoadingStartOfs = mRevsFiles.count();

      const QString runCmd("git diff-tree --no-color -r -C --stdin");
      runAsync(runCmd, this, diffTreeBuf);
   }
}

bool Git::filterEarlyOutputRev(RepositoryModel *fh, Rev *rev)
{

   if (fh->earlyOutputCnt < fh->revOrder.count())
   {

      const QString &sha = fh->revOrder[fh->earlyOutputCnt++];
      const Rev *c = revLookup(sha, fh);
      if (c)
      {
         if (rev->sha() != sha || rev->parents() != c->parents())
         {
            // mismatch found! set correct value, 'rev' will
            // overwrite 'c' upon returning
            rev->orderIdx = c->orderIdx;
            mRevData->clear(false); // flush the tail
         }
         else
            return true; // filter out 'rev'
      }
   }
   // we have new revisions, exit from early output state
   fh->setEarlyOutputState(false);
   return false;
}

int Git::addChunk(RepositoryModel *fh, const QByteArray &ba, int start)
{

   RevMap &r = fh->revs;
   int nextStart;
   Rev *rev;

   do
   {
      // only here we create a new rev
      rev = new Rev(ba, start, fh->revOrder.count(), &nextStart, !isMainHistory(fh));

      if (nextStart == -2)
      {
         delete rev;
         fh->setEarlyOutputState(true);
         start = ba.indexOf('\n', start) + 1;
      }

   } while (nextStart == -2);

   if (nextStart == -1)
   { // half chunk detected
      delete rev;
      return -1;
   }

   const QString &sha = rev->sha();

   if (fh->earlyOutputCnt != -1 && filterEarlyOutputRev(fh, rev))
   {
      delete rev;
      return nextStart;
   }

   if (r.isEmpty() && !isMainHistory(fh))
   {
      bool added = copyDiffIndex(fh, sha);
      rev->orderIdx = added ? 1 : 0;
   }
   if (!isMainHistory(fh) && !fh->renamedPatches.isEmpty() && fh->renamedPatches.contains(sha))
   {

      // this is the new rev with renamed file, the rev is correct but
      // the patch, create a new rev with proper patch and use that instead
      const Rev *prevSha = revLookup(sha, fh);
      Rev *c = fakeRevData(sha, rev->parents(), rev->author(), rev->authorDate(), rev->shortLog(), rev->longLog(),
                           fh->renamedPatches[sha], prevSha->orderIdx, fh);

      r.insert(sha, c); // overwrite old content
      fh->renamedPatches.remove(sha);
      return nextStart;
   }
   if (!isMainHistory(fh) && rev->parentsCount() > 1 && r.contains(sha))
   {
      /* In this case git log is called with -m option and merges are splitted
     in one commit per parent but all them have the same sha.
     So we add only the first to fh->revOrder to display history correctly,
     but we nevertheless add all the commits to 'r' so that annotation code
     can get the patches.
  */
      QString mergeSha;
      int i = 0;
      do
         mergeSha = QString("%1%2%3").arg(++i).arg(" m ").arg(sha);
      while (r.contains(mergeSha));

      const QString &ss = toPersistentSha(mergeSha, mShaBackupBuf);
      r.insert(ss, rev);
   }
   else
   {
      r.insert(sha, rev);
      fh->revOrder.append(sha);

      if (rev->parentsCount() == 0 && !isMainHistory(fh))
         fh->renamedRevs.append(sha);
   }

   return nextStart;
}

bool Git::copyDiffIndex(RepositoryModel *fh, const QString &parent)
{
   // must be called with empty revs and empty revOrder

   if (!fh->revOrder.isEmpty() || !fh->revs.isEmpty())
      return false;

   const Rev *r = revLookup(ZERO_SHA);
   if (!r)
      return false;

   const RevFile *files = getFiles(ZERO_SHA);
   if (!files || findFileIndex(*files, fh->fileNames().first()) == -1)
      return false;

   // insert a custom ZERO_SHA rev with proper parent
   const Rev *rf = fakeWorkDirRev(parent, "Working directory changes", "long log\n", 0, fh);
   fh->revs.insert(ZERO_SHA, rf);
   fh->revOrder.append(ZERO_SHA);
   return true;
}

void Git::setLane(const QString &sha, RepositoryModel *fh)
{

   Lanes *l = fh->lns;
   uint i = fh->firstFreeLane;
   QVector<QByteArray> ba;
   const QString &ss = toPersistentSha(sha, ba);
   const QVector<QString> &shaVec(fh->revOrder);

   for (uint cnt = shaVec.count(); i < cnt; ++i)
   {

      const QString &curSha = shaVec[i];
      Rev *r = const_cast<Rev *>(revLookup(curSha, fh));
      if (r->lanes.count() == 0)
         updateLanes(*r, *l, curSha);

      if (curSha == ss)
         break;
   }
   fh->firstFreeLane = ++i;
}

void Git::updateLanes(Rev &c, Lanes &lns, const QString &sha)
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

   lns.getLanes(c.lanes); // here lanes are snapshotted

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

void Git::appendFileName(RevFile &rf, const QString &name, FileNamesLoader &fl)
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

void Git::updateDescMap(const Rev *r, uint idx, QHash<QPair<uint, uint>, bool> &dm, QHash<uint, QVector<int>> &dv)
{

   QVector<int> descVec;
   if (r->descRefsMaster != -1)
   {

      const Rev *tmp = revLookup(mRevData->revOrder[r->descRefsMaster]);
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

void Git::mergeBranches(Rev *p, const Rev *r)
{

   int r_descBrnMaster = (checkRef(r->sha(), BRANCH | RMT_BRANCH) ? r->orderIdx : r->descBrnMaster);

   if (p->descBrnMaster == r_descBrnMaster || r_descBrnMaster == -1)
      return;

   // we want all the descendant branches, so just avoid duplicates
   const QVector<int> &src1 = revLookup(mRevData->revOrder[p->descBrnMaster])->descBranches;
   const QVector<int> &src2 = revLookup(mRevData->revOrder[r_descBrnMaster])->descBranches;
   QVector<int> dst(src1);
   for (int i = 0; i < src2.count(); i++)
      if (std::find(src1.constBegin(), src1.constEnd(), src2[i]) == src1.constEnd())
         dst.append(src2[i]);

   p->descBranches = dst;
   p->descBrnMaster = p->orderIdx;
}

void Git::mergeNearTags(bool down, Rev *p, const Rev *r, const QHash<QPair<uint, uint>, bool> &dm)
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
   const QVector<QString> &ro = mRevData->revOrder;
   const QString &sha1 = down ? ro[p->descRefsMaster] : ro[p->ancRefsMaster];
   const QString &sha2 = down ? ro[r_descRefsMaster] : ro[r_ancRefsMaster];
   const QVector<int> &src1 = down ? revLookup(sha1)->descRefs : revLookup(sha1)->ancRefs;
   const QVector<int> &src2 = down ? revLookup(sha2)->descRefs : revLookup(sha2)->ancRefs;
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

   const QVector<QString> &ro = mRevData->revOrder;
   if (ro.count() == 0)
      return;

   // we keep the pairs(x, y). Value is true if x is
   // ancestor of y or false if y is ancestor of x
   QHash<QPair<uint, uint>, bool> descMap;
   QHash<uint, QVector<int>> descVect;

   // walk down the tree from latest to oldest,
   // compute children and nearest descendants
   for (int i = 0, cnt = ro.count(); i < cnt; i++)
   {

      uint type = checkRef(ro[i]);
      bool isB = (type & (BRANCH | RMT_BRANCH));
      bool isT = (type & TAG);

      const Rev *r = revLookup(ro[i]);

      if (isB)
      {
         Rev *rr = const_cast<Rev *>(r);
         if (r->descBrnMaster != -1)
         {
            const QString &sha = ro[r->descBrnMaster];
            rr->descBranches = revLookup(sha)->descBranches;
         }
         rr->descBranches.append(i);
      }
      if (isT)
      {
         updateDescMap(r, i, descMap, descVect);
         Rev *rr = const_cast<Rev *>(r);
         rr->descRefs.clear();
         rr->descRefs.append(i);
      }
      for (uint y = 0; y < r->parentsCount(); y++)
      {

         Rev *p = const_cast<Rev *>(revLookup(r->parent(y)));
         if (p)
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
   for (int i = ro.count() - 1; i >= 0; i--)
   {

      const Rev *r = revLookup(ro[i]);
      bool isTag = checkRef(ro[i], TAG);

      if (isTag)
      {
         Rev *rr = const_cast<Rev *>(r);
         rr->ancRefs.clear();
         rr->ancRefs.append(i);
      }
      for (int y = 0; y < r->children.count(); y++)
      {

         Rev *c = const_cast<Rev *>(revLookup(ro[r->children[y]]));
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
