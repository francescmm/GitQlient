#pragma once

/****************************************************************************************
 ** GitQlient is an application to manage and operate one or several Git repositories. With
 ** GitQlient you will be able to add commits, branches and manage all the options Git provides.
 ** Copyright (C) 2020  Francesc Martinez
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
#include <QThread>
#include <QPointer>

class GitBase;
class RevisionsCache;
class GitRepoLoader;
class QCloseEvent;
class QFileSystemWatcher;
class QStackedLayout;
class Controls;
class HistoryWidget;
class DiffWidget;
class BlameWidget;
class MergeWidget;
class QTimer;
class WaitingDlg;
class IRestApi;

enum class ControlsMainViews;

namespace Ui
{
class MainWindow;
}

/*!
 \brief The GitQlientRepo class is the main widget that stores all the subwidgets that act over the repository. This
 class manages the signals between the different big widgets such as the top widget controls, the repository view, diff,
 merge and blame & history view.

*/
class GitQlientRepo : public QFrame
{
   Q_OBJECT

signals:
   /*!
    \brief Signal triggered when the user wants to open a submodule in a new GitQlientRepo view.

    \param submoduleName The submodule name.
   */
   void signalOpenSubmodule(const QString &submoduleName);
   /**
    * @brief signalEditFile Signal triggered when the user wants to edit a file and is running GitQlient from QtCreator.
    * @param fileName The file name
    * @param line The line
    * @param column The column
    */
   void signalEditFile(const QString &fileName, int line, int column);

   /**
    * @brief signalLoadRepo Signal used to trigger the data update in a different thread.
    */
   void signalLoadRepo();

   /**
    * @brief repoOpened Signal triggered when the repo was successfully opened.
    * @param repoPath The absolute path to the repository opened.
    */
   void repoOpened(const QString &repoPath);

public:
   /*!
    \brief Default constructor.

    \param repoPath The path in disk where the repository is located.
    \param parent The parent widget if needed.
   */
   explicit GitQlientRepo(const QString &repoPath, QWidget *parent = nullptr);
   /*!
    \brief Destructor.

   */
   ~GitQlientRepo() override;

   /*!
    \brief Gets the current working dir.

    \return QString The current working dir.
   */
   QString currentDir() const { return mCurrentDir; }
   /*!
    \brief Sets the repository once the widget is created.

    \param newDir The new repository to be opened.
   */
   void setRepository(const QString &newDir);

protected:
   /*!
    \brief Overload of the close event cancel any pending loading.

    \param ce The close event.
   */
   void closeEvent(QCloseEvent *ce) override;

private:
   QString mCurrentDir;
   QSharedPointer<RevisionsCache> mGitQlientCache;
   QSharedPointer<GitBase> mGitBase;
   QSharedPointer<GitRepoLoader> mGitLoader;
   HistoryWidget *mHistoryWidget = nullptr;
   QStackedLayout *mStackedLayout = nullptr;
   Controls *mControls = nullptr;
   HistoryWidget *mRepoWidget = nullptr;
   DiffWidget *mDiffWidget = nullptr;
   BlameWidget *mBlameWidget = nullptr;
   MergeWidget *mMergeWidget = nullptr;
   QTimer *mAutoFetch = nullptr;
   QTimer *mAutoFilesUpdate = nullptr;
   QTimer *mAutoPrUpdater = nullptr;
   QPointer<WaitingDlg> mWaitDlg;
   QFileSystemWatcher *mGitWatcher = nullptr;
   QPair<ControlsMainViews, QWidget *> mPreviousView;
   QSharedPointer<IRestApi> mApi;

   bool mIsInit = false;
   QThread *m_loaderThread;

   /*!
    \brief Updates the UI cache and refreshes the subwidgets.

   */
   void updateCache();
   /*!
    \brief Performs a light UI update triggered by the QFileSystemWatcher.

   */
   void updateUiFromWatcher();
   /*!
    \brief Opens the diff view with the selected commit from the repository view.
    \param currentSha The current selected commit SHA.
   */
   void openCommitDiff(const QString currentSha);
   /*!
    \brief Opens the diff view witht the selected SHAs to compare between them.

    \param shas The list of shas to compare between.
   */
   void openCommitCompareDiff(const QStringList &shas);
   /*!
    \brief Method called when changes are commites through the WIP widget.

    \param ok True if the changes are commited, otherwise false.
   */
   void changesCommitted(bool ok);
   /*!
    \brief Method that sets the watcher for the files in the system.

   */
   void setWatcher();
   /*!
    \brief Clears the views and its subwidgets.

   */
   void clearWindow();
   /*!
    \brief Enables or disables the subwidgets.

    \param enabled True if enable, otherwise false,
   */
   void setWidgetsEnabled(bool enabled);
   /*!
    \brief Shows the history of a file.

    \param fileName The path to the file.
   */
   void showFileHistory(const QString &fileName);

   /*!
    \brief Updates the progess dialog when loading a really huge repository.
   */
   void createProgressDialog();

   /*!
    \brief When the loading finishes this method closes and destroyes the dialog.

   */
   void onRepoLoadFinished();
   /*!
    \brief Loads the view to show the diff of a specific file.

    \param currentSha The current SHA.
    \param previousSha The SHA to compare to.
    \param file The file to show the diff.
   */
   void loadFileDiff(const QString &currentSha, const QString &previousSha, const QString &file);

   /*!
    \brief Shows the history/repository view.

   */
   void showHistoryView();
   /*!
    \brief Shows the Gistory & Blame view.

   */
   void showBlameView();
   /*!
    \brief Shows the diff view. Only accessible if there is at least one open diff.

   */
   void showDiffView();
   /*!
    \brief Configures the merge widget to show the conflicts in that view.

   */
   void showWarningMerge();
   /*!
    * \brief Configures the merge widget when a conflict happens and is due to a cherry-pick. The conflicts are shown in
    * the merge view.
    */
   void showCherryPickConflict();
   /*!
    * \brief Configures the merge widget when a conflict happens and is due to a pull. The conflicts are shown in the
    * merge view.
    */
   void showPullConflict();

   /*!
    \brief Shows the merge view.
   */
   void showMergeView();
   /*!
    \brief Opens the previous view. This method is used when the diff view is closed and GitQlientRepo must return to
    the previous one.

   */
   void showPreviousView();
   /*!
    \brief Updated the WIP in the internal cache for this repository.

   */
   void updateWip();

   /**
    * @brief updateTagsOnCache Updates the remote tags in the cache.
    */
   void updateTagsOnCache();
};
