/*
        Author: Marco Costalba (C) 2005-2007

        Copyright: See COPYING file that comes with this distribution

*/
#ifndef GIT_H
#define GIT_H

#include <RevisionFile.h>
#include <ReferenceType.h>
#include <GitExecResult.h>

#include <QVariant>
#include <QVector>
#include <QSharedPointer>

template<class, class>
struct QPair;

class GitBase;
class RevisionsCache;
class CommitInfo;

class Git : public QObject
{
   Q_OBJECT

signals:
   void signalWipUpdated();

public:
   enum class CommitResetType
   {
      SOFT,
      MIXED,
      HARD
   };

   explicit Git(const QSharedPointer<GitBase> &gitBase, QSharedPointer<RevisionsCache> cache,
                QObject *parent = nullptr);

   QString getCurrentBranch() const;

   /* START STASHES */

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
   bool checkoutFile(const QString &fileName);
   GitExecResult resetFile(const QString &fileName);
   GitExecResult blame(const QString &file, const QString &commitFrom);
   GitExecResult history(const QString &file);
   bool resetCommit(const QString &sha, CommitResetType type);
   /* END COMMIT INFO */

   GitExecResult prune();
   GitExecResult getCommitDiff(const QString &sha, const QString &diffToSha);
   QString getFileDiff(const QString &currentSha, const QString &previousSha, const QString &file);

   RevisionFile getDiffFiles(const QString &sha, const QString &sha2);

   GitExecResult merge(const QString &into, QStringList sources);

   QString getWorkingDir() const;

private:
   QSharedPointer<GitBase> mGitBase;
   QSharedPointer<RevisionsCache> mCache;

   bool updateIndex(const RevisionFile &files, const QStringList &selFiles);
   static const QString quote(const QStringList &sl);
};

#endif
