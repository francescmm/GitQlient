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
class QStackedWidget;
class QStackedLayout;
class Controls;
class WorkInProgressWidget;
class CommitInfoWidget;
class FullDiffWidget;
class FileDiffWidget;
class CommitHistoryWidget;
class RevsView;
class BlameWidget;
class FileDiffHighlighter;
class QTimer;
class ProgressDlg;
class DiffWidget;

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
   FileDiffHighlighter *mDiffHighlighter = nullptr;
   QString mCurrentDir;
   QSharedPointer<Git> mGit;
   CommitHistoryWidget *mRepoWidget = nullptr;
   QStackedWidget *commitStackedWidget = nullptr;
   QStackedWidget *centerStackedWidget = nullptr;
   QStackedLayout *mainStackedLayout = nullptr;
   Controls *mControls = nullptr;
   WorkInProgressWidget *mCommitWidget = nullptr;
   CommitInfoWidget *mRevisionWidget = nullptr;
   DiffWidget *mDiffWidget = nullptr;
   FullDiffWidget *mFullDiffWidget = nullptr;
   FileDiffWidget *mFileDiffWidget = nullptr;
   QFileSystemWatcher *mGitWatcher = nullptr;
   BlameWidget *mBlameWidget = nullptr;
   QTimer *mAutoFetch = nullptr;
   QTimer *mAutoFilesUpdate = nullptr;
   GitQlientRepoConfig mConfig;
   ProgressDlg *mProgressDlg = nullptr;

   void updateCache();
   void updateUiFromWatcher();
   void openCommitDiff();
   void openCommitCompareDiff(const QStringList &shas);
   void changesCommitted(bool ok);
   void onCommitSelected(const QString &goToSha);
   void onAmendCommit(const QString &sha);
   void onFileDiffRequested(const QString &currentSha, const QString &previousSha, const QString &file);
   void setWatcher();
   void clearWindow();
   void setWidgetsEnabled(bool enabled);
   void executeCommand();
   void showFileHistory(const QString &fileName);
   void updateProgressDialog();
   void closeProgressDialog();

   // End of MainWindow refactor
   bool isMatch(const QString &sha, const QString &f, int cn, const QMap<QString, bool> &sm);
};
