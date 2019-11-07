/*
        Author: Marco Costalba (C) 2005-2007

        Copyright: See COPYING file that comes with this distribution

*/
#ifndef GIT_H
#define GIT_H

#include <QObject>
#include <QVariant>
#include <QVector>
#include <QSharedPointer>

#include <RevisionFile.h>

template<class, class>
struct QPair;

class RevisionsCache;
class Revision;
class QRegExp;
class QTextCodec;
class Annotate;
class Domain;
class RepositoryModel;
class Lanes;
class GitAsyncProcess;

static const QString ZERO_SHA = "0000000000000000000000000000000000000000";

struct GitExecResult
{
   GitExecResult() = default;
   GitExecResult(const QPair<bool, QString> &result)
      : success(result.first)
      , output(result.second)
   {
   }
   bool success;
   QVariant output;
};

class Git : public QObject
{
   Q_OBJECT

signals:
   void signalMergeWithConflicts(const QVector<QString> &conflictFiles);

   // TODO: To review
signals:
   void newRevsAdded();
   void loadCompleted();
   void cancelLoading();
   void cancelAllProcesses();

public:
   enum class CommitResetType
   {
      SOFT,
      MIXED,
      HARD
   };

   explicit Git();

   /* START Git CONFIGURATION */
   bool init(const QString &wd, QSharedPointer<RevisionsCache> revCache);
   void init2();
   QString getWorkingDir() const { return mWorkingDir; }
   bool clone(const QString &url, const QString &fullPath);
   bool initRepo(const QString &fullPath);
   /* END Git CONFIGURATION */

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
   bool addTag(const QString &tagName, const QString &tagMessage, const QString &sha, QByteArray &output);
   bool removeTag(const QString &tagName, bool remote);
   bool pushTag(const QString &tagName, QByteArray &output);
   bool getTagCommit(const QString &tagName, QByteArray &output);
   const QString getTagMsg(const QString &sha);

   /*  END  TAGS */

   /* START STASHES */
   QVector<QString> getStashes();
   bool getStashCommit(const QString &stash, QByteArray &output);
   /*  END  STASHES */

   /* START COMMIT WORK */
   bool commitFiles(QStringList &files, const QString &msg, bool amend, const QString &author = QString());
   GitExecResult exportPatch(const QStringList &shaList);
   bool apply(const QString &fileName, bool asCommit = false);
   GitExecResult push(bool force = false);
   GitExecResult pull();
   bool fetch();
   GitExecResult cherryPickCommit(const QString &sha);
   bool pop();
   bool stash();
   bool resetCommit(const QString &sha, CommitResetType type);
   bool resetCommits(int parentDepth);
   GitExecResult checkoutCommit(const QString &sha);
   /* END COMMIT WORK */

   /* START COMMIT INFO */
   QPair<QString, QString> getSplitCommitMsg(const QString &sha);
   QString getCommitMsg(const QString &sha) const;
   const QString getLastCommitMsg();
   const QString getNewCommitMsg();
   bool resetFile(const QString &fileName);
   GitExecResult blame(const QString &file);
   GitExecResult history(const QString &file);
   /* END COMMIT INFO */

   /* START SUBMODULES */
   QVector<QString> getSubmodules();
   bool submoduleAdd(const QString &url, const QString &name);
   bool submoduleUpdate(const QString &submodule);
   bool submoduleRemove(const QString &submodule);
   /*  END  SUBMODULES */

   /* START SETTINGS */
   void userInfo(QStringList &info);

   /* END SETTINGS */

   /* START GENERAL REPO */
   bool getBaseDir(const QString &wd, QString &bd);
   /*  END  GENERAL REPO */

   enum RefType
   {
      TAG = 1,
      BRANCH = 2,
      RMT_BRANCH = 4,
      CUR_BRANCH = 8,
      REF = 16,
      APPLIED = 32,
      UN_APPLIED = 64,
      ANY_REF = 127
   };

   void setDefaultModel(RepositoryModel *fh) { mRevData = fh; }
   void setLane(const QString &sha);
   void cancelDataLoading();

   bool isNothingToCommit();

   void getDiff(const QString &sha, QObject *receiver, const QString &diffToSha, bool combined);
   QString getDiff(const QString &currentSha, const QString &previousSha, const QString &file);

   RevisionFile getWipFiles();
   RevisionFile getFiles(const QString &sha, const QString &sha2 = "", bool all = false);

   const QString getLaneParent(const QString &fromSHA, int laneNum);
   const QStringList getChildren(const QString &parent);
   Revision getRevLookup(const QString &sha) const;
   uint checkRef(const QString &sha, uint mask = ANY_REF) const;
   const QString getRefSha(const QString &refName, RefType type = ANY_REF, bool askGit = true);
   const QStringList getRefNames(const QString &sha, uint mask = ANY_REF) const;
   const QStringList sortShaListByIndex(QStringList &shaList);
   GitExecResult merge(const QString &into, QStringList sources);

   const QString filePath(const RevisionFile &rf, int i) const;
   QPair<bool, QString> run(const QString &cmd) const;

   void updateWipRevision();

private:
   void on_loaded();
   bool getGitDBDir(const QString &wd);

   friend class DataLoader;

   struct Reference
   { // stores tag information associated to a revision
      Reference()
         : type(0)
      {
      }
      uint type;
      QStringList branches;
      QStringList remoteBranches;
      QStringList tags;
      QStringList refs;
      QString tagObj; // TODO support more then one obj
      QString tagMsg;
      QString stgitPatch;
   };

   struct WorkingDirInfo
   {
      void clear()
      {
         diffIndex = diffIndexCached = "";
         otherFiles.clear();
      }
      QString diffIndex;
      QString diffIndexCached;
      QStringList otherFiles;
   };
   WorkingDirInfo workingDirInfo;

   struct FileNamesLoader
   {
      FileNamesLoader()
         : rf(nullptr)
      {
      }

      RevisionFile *rf;
      QVector<int> rfDirs;
      QVector<int> rfNames;
   };
   FileNamesLoader fileLoader;

   bool updateIndex(const QStringList &selFiles);
   const QString getWorkDirDiff(const QString &fileName = "");
   int findFileIndex(const RevisionFile &rf, const QString &name);
   void runAsync(const QString &cmd, QObject *rcv, const QString &buf = "");
   bool getRefs();
   void clearRevs();
   void clearFileNames();
   bool startRevList();
   bool startParseProc(const QStringList &initCmd);
   bool populateRenamedPatches(const QString &sha, const QStringList &nn, QStringList *on, bool bt);
   bool filterEarlyOutputRev(Revision *revision);
   int addChunk(const QByteArray &ba, int ofs);
   void parseDiffFormat(RevisionFile &rf, const QString &buf, FileNamesLoader &fl);
   void parseDiffFormatLine(RevisionFile &rf, const QString &line, int parNum, FileNamesLoader &fl);
   Revision *fakeRevData(const QString &sha, const QStringList &parents, const QString &author, const QString &date,
                         const QString &log, const QString &longLog, const QString &patch, int idx);
   const Revision *fakeWorkDirRev(const QString &parent, const QString &log, const QString &longLog, int idx);
   RevisionFile fakeWorkDirRevFile(const WorkingDirInfo &wd);
   RevisionFile insertNewFiles(const QString &sha, const QString &data);
   RevisionFile getAllMergeFiles(const Revision *r);
   bool runDiffTreeWithRenameDetection(const QString &runCmd, QString *runOutput);
   void updateDescMap(const Revision *r, uint i, QHash<QPair<uint, uint>, bool> &dm, QHash<uint, QVector<int>> &dv);
   void mergeNearTags(bool down, Revision *p, const Revision *r, const QHash<QPair<uint, uint>, bool> &dm);
   void mergeBranches(Revision *p, const Revision *r);
   void updateLanes(Revision &c, Lanes &lns, const QString &sha);
   const QStringList getOthersFiles();
   const QStringList getOtherFiles(const QStringList &selFiles);
   void appendFileName(RevisionFile &rf, const QString &name, FileNamesLoader &fl);
   void flushFileNames(FileNamesLoader &fl);
   void populateFileNamesMap();
   static const QString quote(const QString &nm);
   static const QString quote(const QStringList &sl);
   void setStatus(RevisionFile &rf, const QString &rowSt);
   void setExtStatus(RevisionFile &rf, const QString &rowSt, int parNum, FileNamesLoader &fl);
   void appendNamesWithId(QStringList &names, const QString &sha, const QStringList &data, bool onlyLoaded);
   Reference *lookupReference(const QString &sha);
   Reference *lookupOrAddReference(const QString &sha);

   QString mWorkingDir;
   QString mGitDir;
   QString mCurrentBranchName;
   bool mIsMergeHead = false;
   QHash<QString, Reference> mRefsShaMap;
   QVector<QString> mFileNames;
   QVector<QString> mDirNames;
   QHash<QString, int> mFileNamesMap; // quick lookup file name
   QHash<QString, int> mDirNamesMap; // quick lookup directory name
   RepositoryModel *mRevData = nullptr;
   QSharedPointer<RevisionsCache> mRevCache;
};

#endif
