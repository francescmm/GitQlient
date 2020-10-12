#pragma once

/****************************************************************************************
 ** GitQlient is an application to manage and operate one or several Git repositories. With
 ** GitQlient you will be able to add commits, branches and manage all the options Git provides.
 ** Copyright (C) 2020  Francesc Martinez
 **
 ** LinkedIn: www.linkedin.com/in/cescmm/
 ** Web: www.francescmm.com
 **
 ** This library is free software; you can redistribute it and/or
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
#include <QSet>

class QPinnableTabWidget;
class ConfigWidget;

/*!
 \brief The GitQlient class is the MainWindow of the GitQlient application. Is the widget that stores all the tabs about
 the opened repositories and their submodules. Acts as a bridge between the repository actions performed by the
 ConfigWidget and the GitQlientWidget.

*/
class GitQlient : public QWidget
{
   Q_OBJECT
signals:
   void signalEditDocument(const QString &fileName, int line, int column);

public:
   /*!
    \brief Default constructor. Creates an empty GitQlient instance.

    \param parent The parent widget if needed.
   */
   explicit GitQlient(QWidget *parent = nullptr);
   /*!
    \brief Overloaded constructor. Takes a list of arguments and after parsing them, starts the GitQlient instance.

    \param arguments Command prompt arguments.
    \param parent The parent wiget if needed.
   */
   explicit GitQlient(const QStringList &arguments, QWidget *parent = nullptr);
   /*!
    \brief Destructor.

   */
   ~GitQlient() override;

   /*!
    \brief Set the repositories that will be shown.

    \param repositories
   */
   void setRepositories(const QStringList &repositories);
   /*!
    \brief In case that the GitQlient instance it's already initialize, the user can add arguments to be processed.

    \param arguments The list of arguments.
   */
   void setArgumentsPostInit(const QStringList &arguments);

   /**
    * @brief restorePinnedRepos This method restores the pinned repos from the last session
    * @param pinnedRepos The list of repos to restore
    */
   void restorePinnedRepos();

private:
   QPinnableTabWidget *mRepos = nullptr;
   ConfigWidget *mConfigWidget = nullptr;
   QSet<QString> mCurrentRepos;

   /*!
    \brief This method parses all the arguments and configures GitQlient settings with them. Part of the arguments can
    be a list of repositories to be opened. In that case, the method returns the list of repositories to open.

    \param arguments Arguments from the command prompt.
    \return QStringList Returns the list of repositories to be opened. Empty if none is passed.
   */
   QStringList parseArguments(const QStringList &arguments);
   /*!
    \brief Opens a QFileDialog to select a repository in the local disk.

   */
   void openRepo();
   /*!
    \brief Creates a new GitQlientWidget instance or the repository defined in the \p repoPath value. After that, it
    adds a new tab in the current widget.

    \param repoPath The full path of the repository to be opened.
   */
   void addRepoTab(const QString &repoPath);

   /*!
    \brief Creates a new GitQlientWidget instance or the repository defined in the \p repoPath value. After that, it
    adds a new tab in the current widget.

   \param repoPath The full path of the repository to be opened.
           */
   void addNewRepoTab(const QString &repoPath, bool pinned);
   /*!
    \brief Closes a tab. This implies to close all child widgets and remove cache and configuration for that repository
    until it's opened again.

    \param tabIndex The tab index that triggered the close action.
   */
   void closeTab(int tabIndex);

   /**
    * @brief onSuccessOpen Refreshes the UI for the most used and most recent projects lists.
    * @param fullPath The full path of the project successfully opened.
    */
   void onSuccessOpen(const QString &fullPath);

   /**
    * @brief conditionallyOpenPreConfigDlg Opens the pre-config dialog in case that the repo is open for the very first
    * time.
    */
   void conditionallyOpenPreConfigDlg(const QString &repoPath);
};
