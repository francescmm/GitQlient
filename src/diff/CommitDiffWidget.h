#pragma once

#include <QFrame>

class GitBase;
class QLabel;
class FileListWidget;
class RevisionsCache;

/*!
 \brief The CommitDiffWidget configures the layout to show diffs for a whole commit.

*/
class CommitDiffWidget : public QFrame
{
   Q_OBJECT

signals:
   /*!
    \brief Signal triggered if the user wants to open a file that is part of the current commit diff.

    \param currentSha The current commit SHA.
    \param previousSha The previous commit SHA.
    \param file The full file path.
   */
   void signalOpenFileCommit(const QString &currentSha, const QString &previousSha, const QString &file);
   /*!
    \brief Signal triggered that shows the history of a given \p fileName.

    \param fileName The file name to show the history and blame.
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
