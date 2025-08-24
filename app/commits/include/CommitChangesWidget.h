#pragma once

/****************************************************************************************
 ** GitQlient is an application to manage and operate one or several Git repositories. With
 ** GitQlient you will be able to add commits, branches and manage all the options Git provides.
 ** Copyright (C) 2021  Francesc Martinez
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

#include <QMap>
#include <QWidget>

class QListWidget;
class QListWidgetItem;
class GitCache;
namespace Graph { class Cache; }
class GitBase;
class RevisionFiles;
class FileWidget;
class QCheckBox;

namespace Ui
{
class CommitChangesWidget;
}

class CommitChangesWidget : public QWidget
{
   Q_OBJECT

signals:
   void logReload();
   void changeReverted(const QString &revertedFile);
   void signalShowDiff(const QString &fileName, bool isCached);
   void changesCommitted();
   void unstagedFilesChanged();
   void signalShowFileHistory(const QString &fileName);
   void signalUpdateWip();
   void fileStaged(const QString &fileName);
   void signalReturnToHistory();

public:
   enum class CommitMode
   {
      Wip,
      Amend
   };

   explicit CommitChangesWidget(const QSharedPointer<GitCache> &cache,
                                const QSharedPointer<Graph::Cache> &graphCache,
                                const QSharedPointer<GitBase> &git,
                                QWidget *parent = nullptr);

   ~CommitChangesWidget();

   void configure(const QString &sha);
   void reload();
   void clear();
   void clearStaged();
   void setCommitTitleMaxLength();
   void setCommitMode(CommitMode mode);
   CommitMode getCommitMode() const { return mCommitMode; }

protected:
   struct WipCacheItem
   {
      bool keep = false;
      QListWidgetItem *item = nullptr;
   };

   Ui::CommitChangesWidget *ui = nullptr;
   QSharedPointer<GitCache> mCache;
   QSharedPointer<Graph::Cache> mGraphCache;
   QSharedPointer<GitBase> mGit;
   QListWidget *mUnstagedFilesList = nullptr;
   QListWidget *mStagedFilesList = nullptr;
   QString mCurrentSha;
   QMap<QString, WipCacheItem> mInternalCache;
   int mTitleMaxLength = 50;
   CommitMode mCommitMode = CommitMode::Wip;
   QListWidgetItem *mLastSelectedItem = nullptr;
   QListWidget *mLastSelectedList = nullptr;

private slots:
   void commitChanges();
   void showUnstagedMenu(const QPoint &pos);
   void onAmendToggled(bool checked);

private:
   void configureWipMode(const QString &sha);
   void configureAmendMode(const QString &sha);
   void commitWipChanges();
   void commitAmendChanges();

   void insertFiles(const RevisionFiles &files, QListWidget *fileList);
   QPair<QListWidgetItem *, FileWidget *> fillFileItemInfo(const QString &file, bool isConflict, const QString &icon,
                                                           const QColor &color, QListWidget *parent);
   void prepareCache();
   void clearCache();
   void addAllFilesToCommitList();
   void requestDiff(const QString &fileName);
   QString addFileToCommitList(QListWidgetItem *item, bool updateGit = true);
   void revertAllChanges();
   void removeFileFromCommitList(QListWidgetItem *item);
   QStringList getFiles();
   bool checkMsg(QString &msg);
   void updateCounter(const QString &text);
   bool hasConflicts();
   void resetFile(QListWidgetItem *item);
   QColor getColorForFile(const RevisionFiles &files, int index) const;
   void handleItemClick(QListWidget *list, QListWidgetItem *item);
};
