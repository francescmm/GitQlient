/*
        Author: Marco Costalba (C) 2005-2007

        Copyright: See COPYING file that comes with this distribution

*/
#ifndef GIT_H
#define GIT_H

#include <QObject>
#include "common.h"

template<class, class>
struct QPair;

class RevFile;
class Rev;
class QRegExp;
class QTextCodec;
class Annotate;
class Domain;
class RepositoryModel;
class Lanes;
class MyProcess;

class Git : public QObject
{
   Q_OBJECT

signals:
   void signalMergeWithConflicts(const QVector<QString> &conflictFiles);

   // TODO: To review
signals:
   void newRevsAdded(const RepositoryModel *, const QVector<QString> &);
   void loadCompleted(const RepositoryModel *, const QString &);
   void cancelLoading(const RepositoryModel *);
   void cancelAllProcesses();

public:
   enum class CommitResetType
   {
      SOFT,
      MIXED,
      HARD
   };
   static Git *getInstance(QObject *p = nullptr);

   /** START Git CONFIGURATION **/
   bool init(const QString &wd, const QStringList *args);
   void init2();
   void stop(bool saveCache);

   /** END Git CONFIGURATION **/

   /** START BRANCHES **/
   bool createBranchFromCurrent(const QString &newName, QByteArray &output);
   bool createBranchFromAnotherBranch(const QString &oldName, const QString &newName, QByteArray &output);
   bool createBranchAtCommit(const QString &commitSha, const QString &branchName, QByteArray &output);
   bool checkoutRemoteBranch(const QString &branchName, QByteArray &output);
   bool checkoutNewLocalBranch(const QString &branchName, QByteArray &output);
   bool renameBranch(const QString &oldName, const QString &newName, QByteArray &output);
   bool removeLocalBranch(const QString &branchName, QByteArray &output);
   bool removeRemoteBranch(const QString &branchName, QByteArray &output);
   bool getBranches(QByteArray &output);
   bool getDistanceBetweenBranches(bool toMaster, const QString &right, QByteArray &output);
   bool getBranchesOfCommit(const QString &sha, QByteArray &output);
   bool getLastCommitOfBranch(const QString &branch, QByteArray &sha);
   bool prune(QByteArray &output);
   const QString getCurrentBranchName() const { return mCurrentBranchName; }

   /** END BRANCHES **/

   /** START TAGS **/
   QVector<QString> getTags();
   bool addTag(const QString &tagName, const QString &tagMessage, const QString &sha, QByteArray &output);
   bool removeTag(const QString &tagName, bool remote);
   bool pushTag(const QString &tagName, QByteArray &output);
   bool getTagCommit(const QString &tagName, QByteArray &output);
   const QString getTagMsg(const QString &sha);

   /**  END  TAGS **/

   /** START STASHES **/
   QVector<QString> getStashes();
   bool getStashCommit(const QString &stash, QByteArray &output);
   /**  END  STASHES **/

   /** START COMMIT WORK **/
   bool commitFiles(QStringList &files, const QString &msg, bool amend, const QString &author = QString());
   bool push(bool force = false);
   bool pull(QString &output);
   bool fetch();
   bool cherryPickCommit(const QString &sha);
   bool pop();
   bool stash();
   bool resetCommit(const QString &sha, CommitResetType type);
   const QString getShortLog(const QString &sha);
   bool resetCommits(int parentDepth);
   /** END COMMIT WORK **/

   /** START COMMIT INFO **/
   QPair<QString, QString> getSplitCommitMsg(const QString &sha);
   QString getCommitMsg(const QString &sha) const;
   const QString getLastCommitMsg();
   const QString getNewCommitMsg();
   bool resetFile(const QString &fileName);

   /** END COMMIT INFO **/

   /** START SUBMODULES **/
   QVector<QString> getSubmodules();
   bool submoduleUpdate(const QString &submodule);
   bool submoduleRemove(const QString &submodule);
   /**  END  SUBMODULES **/

   /** START SETTINGS **/
   void userInfo(QStringList &info);
   QStringList getGitConfigList(bool global);

   /** END SETTINGS **/

   /** START GENERAL REPO **/
   bool getBaseDir(const QString &wd, QString &bd, bool &changed);
   /**  END  GENERAL REPO **/

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
   void setLane(const QString &sha, RepositoryModel *fh);
   void cancelDataLoading(const RepositoryModel *fh);

   bool isNothingToCommit();
   bool isMainHistory(const RepositoryModel *fh) { return (fh == mRevData); }

   // TODO: Is not right to return a MyProcess when it's supposed to be async
   MyProcess *getDiff(const QString &sha, QObject *receiver, const QString &diffToSha, bool combined);
   QString getDiff(const QString &currentSha, const QString &previousSha, const QString &file);

   const RevFile *getFiles(const QString &sha, const QString &sha2 = "", bool all = false, const QString &path = "");
   static const QString getLocalDate(const QString &gitDate);

   const QString getLaneParent(const QString &fromSHA, int laneNum);
   const QStringList getChildren(const QString &parent);
   const Rev *revLookup(const QString &sha, const RepositoryModel *fh = nullptr) const;
   uint checkRef(const QString &sha, uint mask = ANY_REF) const;
   const QString getRefSha(const QString &refName, RefType type = ANY_REF, bool askGit = true);
   const QStringList getRefNames(const QString &sha, uint mask = ANY_REF) const;
   const QStringList sortShaListByIndex(QStringList &shaList);
   bool merge(const QString &into, QStringList sources, QString *error = nullptr);

   void addExtraFileInfo(QString *rowName, const QString &sha, const QString &diffToSha, bool allMergeFiles);
   void removeExtraFileInfo(QString *rowName);
   void formatPatchFileHeader(QString *rowName, const QString &sha, const QString &dts, bool cmb, bool all);
   const QString filePath(const RevFile &rf, int i) const { return mDirNames[rf.dirAt(i)] + mFileNames[rf.nameAt(i)]; }
   void setCurContext(Domain *d) { mCurrentDomain = d; }
   Domain *curContext() const { return mCurrentDomain; }
   QPair<bool, QString> run(const QString &cmd);

private:
   void loadFileNames();
   void loadFileCache();
   void on_newDataReady(const RepositoryModel *);
   void on_loaded(RepositoryModel *, ulong, int, bool);
   bool saveOnCache(const QString &gitDir, const RevFileMap &rf, const QVector<QString> &dirs,
                    const QVector<QString> &files);
   bool loadFromCache(const QString &gitDir, RevFileMap &rfm, QVector<QString> &dirs, QVector<QString> &files,
                      QByteArray &revsFilesShaBuf);
   bool getGitDBDir(const QString &wd, QString &gd, bool &changed);

   explicit Git(QObject *parent = nullptr);
   static Git *INSTANCE;

   friend class DataLoader;
   friend class ConsoleImpl;

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
   typedef QHash<QString, Reference> RefMap;

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

      RevFile *rf;
      QVector<int> rfDirs;
      QVector<int> rfNames;
   };
   FileNamesLoader fileLoader;

   bool updateIndex(const QStringList &selFiles);
   const QString getWorkDirDiff(const QString &fileName = "");
   int findFileIndex(const RevFile &rf, const QString &name);
   MyProcess *runAsync(const QString &cmd, QObject *rcv, const QString &buf = "");
   bool getRefs();
   void clearRevs();
   void clearFileNames();
   bool startRevList(QStringList &args, RepositoryModel *fh);
   bool startParseProc(const QStringList &initCmd, RepositoryModel *fh, const QString &buf);
   bool tryFollowRenames(RepositoryModel *fh);
   bool populateRenamedPatches(const QString &sha, const QStringList &nn, RepositoryModel *fh, QStringList *on,
                               bool bt);
   bool filterEarlyOutputRev(RepositoryModel *fh, Rev *rev);
   int addChunk(RepositoryModel *fh, const QByteArray &ba, int ofs);
   void parseDiffFormat(RevFile &rf, const QString &buf, FileNamesLoader &fl);
   void parseDiffFormatLine(RevFile &rf, const QString &line, int parNum, FileNamesLoader &fl);
   void getDiffIndex();
   Rev *fakeRevData(const QString &sha, const QStringList &parents, const QString &author, const QString &date,
                    const QString &log, const QString &longLog, const QString &patch, int idx, RepositoryModel *fh);
   const Rev *fakeWorkDirRev(const QString &parent, const QString &log, const QString &longLog, int idx,
                             RepositoryModel *fh);
   const RevFile *fakeWorkDirRevFile(const WorkingDirInfo &wd);
   bool copyDiffIndex(RepositoryModel *fh, const QString &parent);
   const RevFile *insertNewFiles(const QString &sha, const QString &data);
   const RevFile *getAllMergeFiles(const Rev *r);
   bool runDiffTreeWithRenameDetection(const QString &runCmd, QString *runOutput);
   void indexTree();
   void updateDescMap(const Rev *r, uint i, QHash<QPair<uint, uint>, bool> &dm, QHash<uint, QVector<int>> &dv);
   void mergeNearTags(bool down, Rev *p, const Rev *r, const QHash<QPair<uint, uint>, bool> &dm);
   void mergeBranches(Rev *p, const Rev *r);
   void updateLanes(Rev &c, Lanes &lns, const QString &sha);
   const QStringList getOthersFiles();
   const QStringList getOtherFiles(const QStringList &selFiles);
   const QString getNewestFileName(QStringList &args, const QString &fileName);
   void appendFileName(RevFile &rf, const QString &name, FileNamesLoader &fl);
   void flushFileNames(FileNamesLoader &fl);
   void populateFileNamesMap();
   static const QString quote(const QString &nm);
   static const QString quote(const QStringList &sl);
   void setStatus(RevFile &rf, const QString &rowSt);
   void setExtStatus(RevFile &rf, const QString &rowSt, int parNum, FileNamesLoader &fl);
   void appendNamesWithId(QStringList &names, const QString &sha, const QStringList &data, bool onlyLoaded);
   Reference *lookupReference(const QString &sha);
   Reference *lookupOrAddReference(const QString &sha);

   int exGitStopped;

   Domain *mCurrentDomain = nullptr;
   QString mWorkingDir; // workDir is always without trailing '/'
   QString mGitDir;
   QString mFilesLoadingPending;
   QString mFilesLoadingCurrentSha;
   QString mCurrentBranchName;
   int mFilesLoadingStartOfs = 0;
   bool mCacheNeedsUpdate = false;
   bool mErrorReportingEnabled = true;
   bool mIsMergeHead = false;
   bool mIsGIT = false;
   bool mTextHighlighterFound = false;
   QString mTextHighlighterVersionFound;
   bool mLoadingUnappliedPatches = false;
   bool mFileCacheAccessed = false;
   int mPatchesStillToFind = 0;
   QString mFirstNonStGitPatch;
   RevFileMap mRevsFiles;
   QVector<QByteArray> mRevsFilesShaBackupBuf;
   RefMap mRefsShaMap;
   QVector<QByteArray> mShaBackupBuf;
   QVector<QString> mFileNames;
   QVector<QString> mDirNames;
   QHash<QString, int> mFileNamesMap; // quick lookup file name
   QHash<QString, int> mDirNamesMap; // quick lookup directory name
   RepositoryModel *mRevData = nullptr;
};

#endif
