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

#include <QWidget>
#include <QMap>

class QListWidget;
class QListWidgetItem;
class RevisionsCache;
class GitBase;
class RevisionFiles;

namespace Ui
{
class WorkInProgressWidget;
}

class WorkInProgressWidget : public QWidget
{
   Q_OBJECT

signals:
   void signalShowDiff(const QString &sha, const QString &parentSha, const QString &fileName);
   void signalChangesCommitted(bool commited);
   void signalCheckoutPerformed(bool success);
   void signalShowFileHistory(const QString &fileName);

public:
   explicit WorkInProgressWidget(const QSharedPointer<RevisionsCache> &cache, const QSharedPointer<GitBase> &git,
                                 QWidget *parent = nullptr);

   ~WorkInProgressWidget();

   void configure(const QString &sha);
   void clear();
   bool isAmendActive() const { return mIsAmend; }

private:
   bool mIsAmend = false;
   Ui::WorkInProgressWidget *ui = nullptr;
   QSharedPointer<RevisionsCache> mCache;
   QSharedPointer<GitBase> mGit;
   QString mCurrentSha;
   QMap<QString, QPair<bool, QListWidgetItem *>> mCurrentFilesCache;

   void insertFilesInList(const RevisionFiles &files, QListWidget *fileList);
   void prepareCache();
   void clearCache();
   void addAllFilesToCommitList();
   void onOpenDiffRequested(QListWidgetItem *item);
   void requestDiff(const QString &fileName);
   void addFileToCommitList(QListWidgetItem *item);
   void revertAllChanges();
   void removeFileFromCommitList(QListWidgetItem *item);
   bool commitChanges();
   bool amendChanges();
   void showUnstagedMenu(const QPoint &pos);
   void showUntrackedMenu(const QPoint &pos);
   void showStagedMenu(const QPoint &pos);
   void applyChanges();
   QStringList getFiles();
   bool checkMsg(QString &msg);
   void updateCounter(const QString &text);
   bool hasConflicts();
   void resetInfo(bool force = true);
   void resetFile(QListWidgetItem *item);

   static QString lastMsgBeforeError;
   static const int kMaxTitleChars;
};
