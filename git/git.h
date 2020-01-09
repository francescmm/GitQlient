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
   explicit Git(const QSharedPointer<GitBase> &gitBase, QSharedPointer<RevisionsCache> cache,
                QObject *parent = nullptr);

   /* START LOCAL */
   bool commitFiles(QStringList &files, const QString &msg, bool amend, const QString &author = QString());

   /* END LOCAL */

   /* START COMMIT INFO */
   GitExecResult blame(const QString &file, const QString &commitFrom);
   GitExecResult history(const QString &file);
   /* END COMMIT INFO */

   GitExecResult getCommitDiff(const QString &sha, const QString &diffToSha);
   QString getFileDiff(const QString &currentSha, const QString &previousSha, const QString &file);
   RevisionFile getDiffFiles(const QString &sha, const QString &sha2);

private:
   QSharedPointer<GitBase> mGitBase;
   QSharedPointer<RevisionsCache> mCache;

   bool updateIndex(const RevisionFile &files, const QStringList &selFiles);
   static const QString quote(const QStringList &sl);
};

#endif
