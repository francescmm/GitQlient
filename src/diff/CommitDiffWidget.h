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

class GitBase;
class FileListWidget;
class GitCache;

/*!
 \brief The CommitDiffWidget creates the layout that contains the information of a commit diff. This widget is located
 in the bottom left corner of the Diff view. It is composed by a list with the files that are part of the commit and two
 labels that indicates the current SHA and the SHA that is compared with.

To configure it the user just needs to call the \ref configure method with the two SHA of the commits they want to
compare between.

 \class CommitDiffWidget CommitDiffWidget.h "CommitDiffWidget.h"
*/
class CommitDiffWidget : public QFrame
{
   Q_OBJECT

signals:
   /*!
    \brief Signal triggered when the user double clicks a file in the list. This notifies the general Diff view that the
    user wants to open a file.

    \param currentSha The current commit SHA.
    \param previousSha The parent SHA of the current commit.
    \param file The file to show the diff.
   */
   void signalOpenFileCommit(const QString &currentSha, const QString &previousSha, const QString &file, bool isCached);

   /*!
    \brief Signal triggered when the user whats to see the blame history for a given file.

    \param fileName The file name to blame.
   */
   void signalShowFileHistory(const QString &fileName);

   /**
    * @brief signalEditFile Signal triggered when the user wants to edit a file and is running GitQlient from QtCreator.
    * @param fileName The file name
    * @param line The line
    * @param column The column
    */
   void signalEditFile(const QString &fileName, int line, int column);

public:
   /*!
    \brief Default constructor.

    \param git The git object to perform Git operations.
    \param cache The repository internal cache.
    \param parent The parent widget if needed.
   */
   explicit CommitDiffWidget(QSharedPointer<GitBase> git, QSharedPointer<GitCache> cache, QWidget *parent = nullptr);

   /*!
    \brief Configures the widget by passing the two SHAs that will be compared.

    \param firstSha The first SHA or base.
    \param secondSha The second SHA.
   */
   void configure(const QString &firstSha, const QString &secondSha);

private:
   QSharedPointer<GitBase> mGit;
   QSharedPointer<GitCache> mCache;
   FileListWidget *fileListWidget = nullptr;
   QString mFirstShaStr;
   QString mSecondShaStr;
};
