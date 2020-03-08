#pragma once

#include <QFrame>
#include <QMap>

class GitBase;
class QStackedWidget;
class DiffButton;
class QVBoxLayout;
class CommitDiffWidget;
class RevisionsCache;

/*!
 \brief The DiffWidget class creates the layout to display the dif information for both files and commits.

*/
class DiffWidget : public QFrame
{
   Q_OBJECT

signals:
   /*!
    \brief Signal triggered when the user whants to see the history and blame of a given file.

    \param fileName The full file name.
   */
   void signalShowFileHistory(const QString &fileName);
   /*!
    \brief Signal triggered when the user close the last diff opened. This signal is used to disable the button in the
    Controls widget and return to the last view.

   */
   void signalDiffEmpty();

public:
   /*!
    \brief Default constructor.

    \param git The git object to perform Git operations.
    \param cache The internal repository cache for the repository.
    \param parent The parent wiget if needed.
   */
   explicit DiffWidget(const QSharedPointer<GitBase> git, QSharedPointer<RevisionsCache> cache,
                       QWidget *parent = nullptr);
   /*!
    \brief Destructor

   */
   ~DiffWidget() override;

   /*!
    \brief Reloads the information currently shown in the diff.

   */
   void reload();

   /*!
    \brief Clears the information in the current diff.

   */
   void clear() const;
   /*!
    \brief Loads a file diff.

    \param sha The current SHA as base.
    \param previousSha The SHA to compare to.
    \param file The file to show the diff of.
    \return bool Returns true if the file diff was loaded correctly.
   */
   bool loadFileDiff(const QString &sha, const QString &previousSha, const QString &file);
   /*!
    \brief Loads a full commit diff.

    \param sha The base SHA.
    \param parentSha The SHA to compare to.
   */
   void loadCommitDiff(const QString &sha, const QString &parentSha);

private:
   QSharedPointer<GitBase> mGit;
   QStackedWidget *centerStackedWidget = nullptr;
   QMap<QString, QPair<QFrame *, DiffButton *>> mDiffButtons;
   QVBoxLayout *mDiffButtonsContainer = nullptr;
   CommitDiffWidget *mCommitDiffWidget = nullptr;

   /*!
    \brief When the user selectes a different diff from a different tab, it triggers an actionto change the current
    DiffButton selection.

    \param index The new selected index.
   */
   void changeSelection(int index);
};
