#include "GitBase.h"

#include <RevisionsCache.h>
#include <GitRequestorProcess.h>
#include <GitSyncProcess.h>

#include <QLogger.h>
using namespace QLogger;

#include <QDir>

static const QString GIT_LOG_FORMAT = "%m%HX%P%n%cn<%ce>%n%an<%ae>%n%at%n%s%n%b";

GitBase::GitBase(QObject *parent)
   : QObject(parent)
   , mRevCache(new RevisionsCache())
{
}

QPair<bool, QString> GitBase::run(const QString &runCmd) const
{
   QString runOutput;
   GitSyncProcess p(mWorkingDir);
   connect(this, &GitBase::cancelAllProcesses, &p, &AGitProcess::onCancel);

   const auto ret = p.run(runCmd, runOutput);

   return qMakePair(ret, runOutput);
}

bool GitBase::loadRepository(const QString &wd)
{
   if (!isLoading)
   {
      QLog_Info("Git", "Initializing Git...");

      // normally called when changing git directory. Must be called after stop()
      clearRevs();

      const auto isGIT = setGitDbDir(wd);

      if (!isGIT)
         return false;

      isLoading = true;

      setBaseDir(wd);

      loadReferences();

      loadRevisions();

      QLog_Info("Git", "... Git init finished");

      return true;
   }

   return false;
}

void GitBase::clearRevs()
{
   mRevCache->clear();
   mRevCache->clearRevisionFile();
   workingDirInfo.clear();
}

bool GitBase::setGitDbDir(const QString &wd)
{
   auto tmp = mWorkingDir;
   mWorkingDir = wd;

   const auto success = run("git rev-parse --git-dir"); // run under newWorkDir
   mWorkingDir = tmp;

   const auto runOutput = success.second.trimmed();

   if (success.first)
   {
      QDir d(runOutput.startsWith("/") ? runOutput : wd + "/" + runOutput);
      mGitDir = d.absolutePath();
   }

   return success.first;
}

void GitBase::setBaseDir(const QString &wd)
{
   auto tmp = mWorkingDir;
   mWorkingDir = wd;

   const auto ret = run("git rev-parse --show-cdup");
   mWorkingDir = tmp;

   auto baseDir = wd;

   if (ret.first)
   {
      QDir d(QString("%1/%2").arg(wd, ret.second.trimmed()));
      baseDir = d.absolutePath();
   }

   if (ret.first)
      mWorkingDir = baseDir;
}

bool GitBase::loadReferences()
{
   const auto branchLoaded = loadCurrentBranch();

   if (branchLoaded)
   {
      const auto ret3 = run("git show-ref -d");

      if (ret3.first)
      {
         mRevCache->clearReferences();

         auto ret = run("git rev-parse HEAD");

         if (ret.first)
            ret.second.remove(ret.second.count() - 1, ret.second.count());

         QString prevRefSha;
         const auto curBranchSHA = ret.second;
         const auto referencesList = ret3.second.split('\n', QString::SkipEmptyParts);

         for (auto reference : referencesList)
         {
            const auto revSha = reference.left(40);
            const auto refName = reference.mid(41);

            // one Revision could have many tags
            auto cur = mRevCache->getReference(revSha);
            cur.configure(refName, curBranchSHA == revSha, prevRefSha);

            mRevCache->insertReference(revSha, std::move(cur));

            if (refName.startsWith("refs/tags/") && refName.endsWith("^{}") && !prevRefSha.isEmpty())
               mRevCache->removeReference(prevRefSha);

            prevRefSha = revSha;
         }

         // mark current head (even when detached)
         auto cur = mRevCache->getReference(curBranchSHA);
         cur.type |= CUR_BRANCH;
         mRevCache->insertReference(curBranchSHA, std::move(cur));

         return mRevCache->countReferences() > 0;
      }
   }

   return false;
}

bool GitBase::loadCurrentBranch()
{
   const auto ret2 = run("git branch");

   if (!ret2.first)
      return false;

   const auto branches = ret2.second.trimmed().split('\n');
   for (auto branch : branches)
   {
      if (branch.startsWith("*"))
      {
         mCurrentBranchName = branch.remove("*").trimmed();
         break;
      }
   }

   if (mCurrentBranchName.contains(" detached "))
      mCurrentBranchName = "";

   return true;
}

void GitBase::loadRevisions()
{
   const auto baseCmd = QString("git log --date-order --no-color --log-size --parents --boundary -z --pretty=format:")
                            .append(GIT_LOG_FORMAT)
                            .append(" --all");

   const auto requestor = new GitRequestorProcess(mWorkingDir);
   connect(requestor, &GitRequestorProcess::procDataReady, this, &GitBase::processRevision);
   connect(this, &GitBase::cancelAllProcesses, requestor, &AGitProcess::onCancel);

   QString buf;
   requestor->run(baseCmd, buf);
}

void GitBase::processRevision(const QByteArray &ba)
{
   QByteArray auxBa = ba;
   const auto commits = ba.split('\000');
   const auto totalCommits = commits.count();
   auto count = 1;

   mRevCache->configure(totalCommits);

   emit signalLoadingStarted();

   updateWipRevision();

   for (const auto &commitInfo : commits)
   {
      CommitInfo revision(commitInfo, count++);

      if (revision.isValid())
         mRevCache->insertCommitInfo(std::move(revision));
      else
         break;
   }

   isLoading = false;

   emit signalLoadingFinished();
}

void GitBase::updateWipRevision()
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
   const auto log = isNothingToCommit() ? QString("No local changes") : QString("Local changes");
   CommitInfo c(ZERO_SHA, { head }, "-", QDateTime::currentDateTime().toSecsSinceEpoch(), log, status, 0);
   c.isDiffCache = true;

   mRevCache->updateWipCommit(std::move(c), workingDirInfo);
}

bool GitBase::isNothingToCommit()
{
   if (!mRevCache->containsRevisionFile(ZERO_SHA))
      return true;

   const auto rf = mRevCache->getRevisionFile(ZERO_SHA);
   return rf.count() == workingDirInfo.otherFiles.count();
}

QStringList GitBase::getOthersFiles()
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

const QString GitBase::quote(const QString &nm)
{
   return ("$" + nm + "$");
}

// CT TODO utility function; can go elsewhere
const QString GitBase::quote(const QStringList &sl)
{
   QString q(sl.join(QString("$%1$").arg(' ')));
   q.prepend("$").append("$");
   return q;
}
