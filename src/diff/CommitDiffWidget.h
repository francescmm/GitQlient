#pragma once

#include <QFrame>

class GitBase;
class QLabel;
class FileListWidget;
class RevisionsCache;

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
   void signalOpenFileCommit(const QString &currentSha, const QString &previousSha, const QString &file);
   /*!
    \brief Signal triggered when the user whats to see the blame history for a given file.

    \param fileName The file name to blame.
   */
   void signalShowFileHistory(const QString &fileName);

public:
   /*!
    \brief Default constructor.

    \param git The git object to perform Git operations.
    \param cache The repository internal cache.
    \param parent The parent widget if needed.
   */
   explicit CommitDiffWidget(QSharedPointer<GitBase> git, QSharedPointer<RevisionsCache> cache,
                             QWidget *parent = nullptr);

   /*!
    \brief Configures the widget by passing the two SHAs that will be compared.

    \param firstSha The first SHA or base.
    \param secondSha The second SHA.
   */
   void configure(const QString &firstSha, const QString &secondSha);

private:
   QSharedPointer<GitBase> mGit;
   QLabel *mFirstSha = nullptr;
   QLabel *mSecondSha = nullptr;
   FileListWidget *fileListWidget = nullptr;
   QString mFirstShaStr;
   QString mSecondShaStr;
};
