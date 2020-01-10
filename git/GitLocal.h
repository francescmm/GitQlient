#pragma once

/****************************************************************************************
 ** GitQlient is an application to manage and operate one or several Git repositories. With
 ** GitQlient you will be able to add commits, branches and manage all the options Git provides.
 ** Copyright (C) 2019  Francesc Martinez
 **
 ** LinkedIn: www.linkedin.com/in/cescmm/
 ** Web: www.francescmm.com
 **
 ** This program is free software; you can redistribute it and/or
 ** modify it under the terms of the GNU Lesser General Public
 ** License as published by the Free Software Foundation; either
 ** version 2 of the License, or (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 ** Lesser General Public License for more details.
 **
 ** You should have received a copy of the GNU Lesser General Public
 ** License along with this library; if not, write to the Free Software
 ** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 ***************************************************************************************/

#include <GitExecResult.h>

#include <QSharedPointer>

class GitBase;
class RevisionFile;

class GitLocal : public QObject
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

   explicit GitLocal(const QSharedPointer<GitBase> &gitBase);
   GitExecResult cherryPickCommit(const QString &sha);
   GitExecResult checkoutCommit(const QString &sha);
   GitExecResult markFileAsResolved(const QString &fileName);
   bool checkoutFile(const QString &fileName);
   GitExecResult resetFile(const QString &fileName);
   bool resetCommit(const QString &sha, CommitResetType type);
   GitExecResult commitFiles(QStringList &selFiles, const RevisionFile &allCommitFiles, const QString &msg, bool amend,
                             const QString &author = QString());

private:
   QSharedPointer<GitBase> mGitBase;

   GitExecResult updateIndex(const RevisionFile &files, const QStringList &selFiles);
};
