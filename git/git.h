/*
        Author: Marco Costalba (C) 2005-2007

        Copyright: See COPYING file that comes with this distribution

*/
#ifndef GIT_H
#define GIT_H

#include <WorkingDirInfo.h>
#include <RevisionFile.h>
#include <ReferenceType.h>

#include <QObject>
#include <QVariant>
#include <QVector>
#include <QSharedPointer>

template<class, class>
struct QPair;

class RevisionsCache;
class CommitInfo;

static const QString ZERO_SHA = "0000000000000000000000000000000000000000";

struct GitExecResult
{
   GitExecResult() = default;
   GitExecResult(const QPair<bool, QVariant> &result)
      : success(result.first)
      , output(result.second)
   {
   }
   GitExecResult(const QPair<bool, QString> &result)
      : success(result.first)
      , output(result.second)
   {
   }
   GitExecResult &operator=(const QPair<bool, QString> &result)
   {
      success = result.first;
      output = result.second;

      return *this;
   }
   bool success;
   QVariant output;
};

struct GitUserInfo
{
   QString mUserName;
   QString mUserEmail;

   bool isValid() const;
};

class Git : public QObject
{
   Q_OBJECT

signals:
   void cancelAllProcesses();
   void signalLoadingStarted();
   void signalLoadingFinished();
   void signalCloningProgress(QString stepDescription, int value);

public:
   enum class CommitResetType
   {
      SOFT,
      MIXED,
      HARD
   };

   explicit Git();

   /* START Git CONFIGURATION */
   void setWorkingDirectory(const QString &wd) { mWorkingDir = wd; }
   bool loadRepository(const QString &wd);
   QString getWorkingDir() const { return mWorkingDir; }
   bool clone(const QString &url, const QString &fullPath);
   bool initRepo(const QString &fullPath);
   GitUserInfo getGlobalUserInfo() const;
   void setGlobalUserInfo(const GitUserInfo &info);
   GitUserInfo getLocalUserInfo() const;
   void setLocalUserInfo(const GitUserInfo &info);
   /* END Git CONFIGURATION */

   /* START CACHE */
   int totalCommits() const;
   CommitInfo getCommitInfoByRow(int row) const;
   CommitInfo getCommitInfo(const QString &sha);
   /*  END  CACHE */

   /* START BRANCHES */
   GitExecResult createBranchFromAnotherBranch(const QString &oldName, const QString &newName);
   GitExecResult createBranchAtCommit(const QString &commitSha, const QString &branchName);
   GitExecResult checkoutRemoteBranch(const QString &branchName);
   GitExecResult checkoutNewLocalBranch(const QString &branchName);
   GitExecResult renameBranch(const QString &oldName, const QString &newName);
   GitExecResult removeLocalBranch(const QString &branchName);
   GitExecResult removeRemoteBranch(const QString &branchName);
   GitExecResult getBranches();
   GitExecResult getDistanceBetweenBranches(bool toMaster, const QString &right);
   GitExecResult getBranchesOfCommit(const QString &sha);
   GitExecResult getLastCommitOfBranch(const QString &branch);
   GitExecResult prune();
   const QString getCurrentBranchName() const { return mCurrentBranchName; }

   /* END BRANCHES */

   /* START TAGS */
   QVector<QString> getTags() const;
   QVector<QString> getLocalTags() const;
   GitExecResult addTag(const QString &tagName, const QString &tagMessage, const QString &sha);
   GitExecResult removeTag(const QString &tagName, bool remote);
   GitExecResult pushTag(const QString &tagName);
   GitExecResult getTagCommit(const QString &tagName);
   /*  END  TAGS */

   /* START STASHES */
   QVector<QString> getStashes();
   GitExecResult pop() const;
   GitExecResult stash();
   GitExecResult stashBranch(const QString &stashId, const QString &branchName);
   GitExecResult stashDrop(const QString &stashId);
   GitExecResult stashClear();
   bool resetCommit(const QString &sha, CommitResetType type);
   /*  END  STASHES */

   /* START COMMIT WORK */
   bool commitFiles(QStringList &files, const QString &msg, bool amend, const QString &author = QString());
   GitExecResult exportPatch(const QStringList &shaList);
   bool apply(const QString &fileName, bool asCommit = false);
   GitExecResult push(bool force = false);
   GitExecResult pull();
   bool fetch();
   GitExecResult cherryPickCommit(const QString &sha);
   bool resetCommits(int parentDepth);
   GitExecResult checkoutCommit(const QString &sha);
   GitExecResult markFileAsResolved(const QString &fileName);
   /* END COMMIT WORK */

   /* START COMMIT INFO */
   QPair<QString, QString> getSplitCommitMsg(const QString &sha);
   bool checkoutFile(const QString &fileName);
   GitExecResult resetFile(const QString &fileName);
   GitExecResult blame(const QString &file, const QString &commitFrom);
   GitExecResult history(const QString &file);
   /* END COMMIT INFO */

   /* START SUBMODULES */
   QVector<QString> getSubmodules();
   bool submoduleAdd(const QString &url, const QString &name);
   bool submoduleUpdate(const QString &submodule);
   bool submoduleRemove(const QString &submodule);
   /*  END  SUBMODULES */

   bool isNothingToCommit();

   GitExecResult getCommitDiff(const QString &sha, const QString &diffToSha);
   QString getFileDiff(const QString &currentSha, const QString &previousSha, const QString &file);

   RevisionFile getWipFiles();
   RevisionFile getCommitFiles(const QString &sha) const;
   RevisionFile getDiffFiles(const QString &sha, const QString &sha2, bool all = false);

   CommitInfo getCommitInfo(const QString &sha) const;
   uint checkRef(const QString &sha, uint mask = ANY_REF) const;
   const QStringList getRefNames(const QString &sha, uint mask = ANY_REF) const;
   GitExecResult merge(const QString &into, QStringList sources);

   QPair<bool, QString> run(const QString &cmd) const;

   void updateWipRevision();

private:
   bool isLoading = false;
   QString mWorkingDir;
   QString mGitDir;
   QString mCurrentBranchName;
   QSharedPointer<RevisionsCache> mRevCache;
   WorkingDirInfo workingDirInfo;

   bool setGitDbDir(const QString &wd);
   void processRevision(const QByteArray &ba);
   bool loadCurrentBranch();
   bool updateIndex(const QStringList &selFiles);
   bool getRefs();
   void clearRevs();
   bool checkoutRevisions();
   const QStringList getOthersFiles();
   const QStringList getOtherFiles(const QStringList &selFiles);
   static const QString quote(const QString &nm);
   static const QString quote(const QStringList &sl);
   GitExecResult getBaseDir(const QString &wd);
};

#endif
