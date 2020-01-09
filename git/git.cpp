/*
        Description: interface to git programs

        Author: Marco Costalba (C) 2005-2007

        Copyright: See COPYING file that comes with this distribution

*/
#include "git.h"

#include <RevisionsCache.h>
#include <CommitInfo.h>
#include <lanes.h>
#include <GitSyncProcess.h>
#include <GitCloneProcess.h>
#include <GitRequestorProcess.h>
#include <GitBase.h>

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

Git::Git(const QSharedPointer<GitBase> &gitBase, QSharedPointer<RevisionsCache> cache, QObject *parent)
   : QObject(parent)
   , mGitBase(gitBase)
   , mCache(cache)
{
}

GitExecResult Git::getDiffFiles(const QString &sha, const QString &diffToSha)
{
   QString runCmd = QString("git diff-tree -C --no-color -r -m ");

   if (!diffToSha.isEmpty() && sha != CommitInfo::ZERO_SHA)
      runCmd.append(diffToSha + " " + sha);

   return mGitBase->run(runCmd);
}

bool Git::updateIndex(const RevisionFile &files, const QStringList &selFiles)
{
   QStringList toAdd, toRemove;

   for (auto it : selFiles)
   {
      const auto idx = mCache->findFileIndex(files, it);

      idx != -1 && files.statusCmp(idx, RevisionFile::DELETED) ? toRemove << it : toAdd << it;
   }

   if (!toRemove.isEmpty() && !mGitBase->run("git rm --cached --ignore-unmatch -- " + quote(toRemove)).first)
      return false;

   if (!toAdd.isEmpty() && !mGitBase->run("git add -- " + quote(toAdd)).first)
      return false;

   return true;
}

bool Git::commitFiles(QStringList &selFiles, const QString &msg, bool amend, const QString &author)
{
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
   const auto commit = mCache->getCommitInfo(CommitInfo::ZERO_SHA);
   const auto files = mCache->getRevisionFile(CommitInfo::ZERO_SHA, commit.parent(0));
   QStringList notSel;
   for (auto i = 0; i < files.count(); ++i)
   {
      const QString &fp = files.getFile(i);
      if (selFiles.indexOf(fp) == -1 && files.statusCmp(i, RevisionFile::IN_INDEX))
         notSel.append(fp);
   }

   // call git reset to remove not selected files from index
   const auto updIdx = updateIndex(files, selFiles);
   if ((!notSel.empty() && !mGitBase->run("git reset -- " + quote(notSel)).first) || !updIdx
       || !mGitBase->run(QString("git commit" + cmtOptions + " -m \"%1\"").arg(msg)).first
       || (!notSel.empty() && !updIdx))
   {
      ret = false;
   }

   return ret;
}

// CT TODO utility function; can go elsewhere
const QString Git::quote(const QStringList &sl)
{
   QString q(sl.join(QString("$%1$").arg(' ')));
   q.prepend("$").append("$");
   return q;
}
