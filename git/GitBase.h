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
#include <RevisionsCache.h>

#include <QObject>
#include <QSharedPointer>

class GitBase : public QObject
{
   Q_OBJECT

signals:
   void cancelAllProcesses();
   void signalLoadingStarted();
   void signalLoadingFinished();

public:
   explicit GitBase(QObject *parent = nullptr);

   void setWorkingDirectory(const QString &wd) { mWorkingDir = wd; }
   bool loadRepository(const QString &wd);
   QString getWorkingDir() const { return mWorkingDir; }
   void updateWipRevision();
   bool pendingLocalChanges();

protected:
   QSharedPointer<RevisionsCache> mRevCache;
   QString mWorkingDir;

   QPair<bool, QString> run(const QString &cmd) const;

private:
   bool mIsLoading = false;

   bool configureRepoDirectory(const QString &wd);
   void loadReferences();
   void requestRevisions();
   void processRevision(const QByteArray &ba);
   QVector<QString> getUntrackedFiles() const;
};
