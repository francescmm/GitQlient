#pragma once

/****************************************************************************************
 ** GitQlient is an application to manage and operate one or several Git repositories. With
 ** GitQlient you will be able to add commits, branches and manage all the options Git provides.
 ** Copyright (C) 2019  Francesc Martinez
 **
 ** LinkedIn: www.linkedin.com/in/cescmm/
 ** Web: www.francescmm.com
 **
 ** This library is free software; you can redistribute it and/or
 ** modify it under the terms of the GNU Lesser General Public
 ** License as published by the Free Software Foundation; either
 ** version 3 of the License, or (at your option) any later version.
 **
 ** This library is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 ** Lesser General Public License for more details.
 **
 ** You should have received a copy of the GNU Lesser General Public
 ** License along with this library; if not, write to the Free Software
 ** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 ***************************************************************************************/

#include <QFrame>

class QCloseEvent;
class QFileSystemWatcher;
class QListWidgetItem;
class RevsView;
class FileDiffHighlighter;
class Git;

namespace Ui
{
class MainWindow;
}

class MainWindow : public QFrame
{
   Q_OBJECT

signals:
   void closeAllWindows();

public:
   explicit MainWindow(QWidget *parent = nullptr);
   explicit MainWindow(const QString &repo, QWidget *parent = nullptr);

   void setRepository(const QString &newDir);

protected:
   void closeEvent(QCloseEvent *ce) override;

private:
   Ui::MainWindow *ui = nullptr;
   FileDiffHighlighter *mDiffHighlighter = nullptr;
   QString mCurrentDir;
   bool mRepositoryBusy = false;

   void updateUi();
   void goToCommitSha(const QString &goToSha);
   void openCommitDiff();
   void changesCommitted(bool ok);
   void onCommitClicked(const QModelIndex &index);
   void onCommitSelected(const QString &sha);
   void onAmmendCommit(const QString &sha);
   void onFileDiffRequested(const QString &currentSha, const QString &previousSha, const QString &file);
   void resetWatcher(const QString &oldDir, const QString &newDir);
   void clearWindow(bool deepClear);
   void setWidgetsEnabled(bool enabled);
   void executeCommand();

   RevsView *rv = nullptr;
   QFileSystemWatcher *mGitWatcher = nullptr;
   Git *mGit;

   // End of MainWindow refactor

   void rebase(const QString &from, const QString &to, const QString &onto);
   void merge(const QStringList &shas, const QString &into);
   void moveRef(const QString &refName, const QString &toSHA);
   bool isMatch(const QString &sha, const QString &f, int cn, const QMap<QString, bool> &sm);
};
