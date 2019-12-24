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

#include <QFrame>

class Git;
class QCloseEvent;
class QFileSystemWatcher;
class QStackedLayout;
class Controls;
class HistoryWidget;
class DiffWidget;
class BlameWidget;
class MergeWidget;
class QTimer;
class ProgressDlg;

namespace Ui
{
class MainWindow;
}

struct GitQlientRepoConfig
{
   int mAutoFetchSecs = 300;
   int mAutoFileUpdateSecs = 10;
};

class GitQlientRepo : public QFrame
{
   Q_OBJECT

signals:
   void closeAllWindows();
   void signalOpenSubmodule(const QString &submoduleName);
   void signalRepoOpened();

public:
   explicit GitQlientRepo(QWidget *parent = nullptr);

   bool isOpened();
   void setConfig(const GitQlientRepoConfig &config);
   QString currentDir() const { return mCurrentDir; }
   void setRepository(const QString &newDir);
   void close();

protected:
   void closeEvent(QCloseEvent *ce) override;

private:
   QString mCurrentDir;
   GitQlientRepoConfig mConfig;
   QSharedPointer<Git> mGit;
   Controls *mControls = nullptr;
   QStackedLayout *mStackedLayout = nullptr;
   HistoryWidget *mRepoWidget = nullptr;
   DiffWidget *mDiffWidget = nullptr;
   BlameWidget *mBlameWidget = nullptr;
   MergeWidget *mMergeWidget = nullptr;
   QTimer *mAutoFetch = nullptr;
   QTimer *mAutoFilesUpdate = nullptr;
   ProgressDlg *mProgressDlg = nullptr;
   QFileSystemWatcher *mGitWatcher = nullptr;

   void updateCache();
   void updateUiFromWatcher();
   void openCommitDiff();
   void openCommitCompareDiff(const QStringList &shas);
   void changesCommitted(bool ok);
   void setWatcher();
   void clearWindow();
   void setWidgetsEnabled(bool enabled);
   void executeCommand();
   void showFileHistory(const QString &fileName);
   void updateProgressDialog();
   void closeProgressDialog();
   void loadFileDiff(const QString &currentSha, const QString &previousSha, const QString &file);
   void showHistoryView();
   void showBlameView();
   void showDiffView();

   // End of MainWindow refactor
   bool isMatch(const QString &sha, const QString &f, int cn, const QMap<QString, bool> &sm);
};
