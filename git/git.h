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

public:
   explicit Git(const QSharedPointer<GitBase> &gitBase, QSharedPointer<RevisionsCache> cache,
                QObject *parent = nullptr);

   bool commitFiles(QStringList &files, const QString &msg, bool amend, const QString &author = QString());
   GitExecResult getDiffFiles(const QString &sha, const QString &sha2);

private:
   QSharedPointer<GitBase> mGitBase;
   QSharedPointer<RevisionsCache> mCache;

   bool updateIndex(const RevisionFile &files, const QStringList &selFiles);
   static const QString quote(const QStringList &sl);
};

#endif
