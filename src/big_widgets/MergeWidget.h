#pragma once

#include <QFrame>
#include <QMap>

class GitBase;
class QVBoxLayout;
class QPushButton;
class QStackedWidget;
class MergeInfoWidget;
class QLineEdit;
class QTextEdit;
class FileDiffWidget;
class RevisionFiles;
class RevisionsCache;
class ConflictButton;

/**
 * @brief The MergeWidget class creates the layout for when a merge happens. The layout is composed by two lists of
 * ConflictButton in the left side: one for the conflict files and the other for the auto-merged files. Below this lists
 * appears the description for the merge message and two buttons: abort merge and commit.
 *
 * In the center and right part of the view, there are shown the files that the user selects using the ConflictButton
 * buttons.
 *
 */
class MergeWidget : public QFrame
{
   Q_OBJECT

signals:
   /**
    * @brief Signal triggered when the merge ends. It can be by aborting it or by commiting it.
    *
    */
   void signalMergeFinished();

public:
   /**
    * @brief Default constructor.
    *
    * @param gitQlientCache The internal cache for the current repository.
    * @param git The git object to perform Git operations.
    * @param parent The parent widget if needed.
    */
   explicit MergeWidget(const QSharedPointer<RevisionsCache> &gitQlientCache, const QSharedPointer<GitBase> &git,
                        QWidget *parent = nullptr);

   /**
    * @brief Configures the merge widget by giving the current revisions files that are part of the merge.
    *
    * @param files The RevisionFiles data.
    */
   void configure(const RevisionFiles &files);

private:
   QSharedPointer<RevisionsCache> mGitQlientCache;
   QSharedPointer<GitBase> mGit;
   QVBoxLayout *mConflictBtnContainer = nullptr;
   QVBoxLayout *mAutoMergedBtnContainer = nullptr;
   QStackedWidget *mCenterStackedWidget = nullptr;
   QLineEdit *mCommitTitle = nullptr;
   QTextEdit *mDescription = nullptr;
   QPushButton *mMergeBtn = nullptr;
   QPushButton *mAbortBtn = nullptr;
   QMap<ConflictButton *, FileDiffWidget *> mConflictButtons;

   /**
    * @brief Fills both lists of ConflictButton.
    *
    * @param files The RevisionFiles data that contains the list of files.
    */
   void fillButtonFileList(const RevisionFiles &files);
   /**
    * @brief Changes the current diff view of a file when a button is clicked.
    *
    * @param fileBtnChecked True if the ConflictButton is selected.
    */
   void changeDiffView(bool fileBtnChecked);
   /**
    * @brief Aborts the current merge.
    *
    */
   void abort();
   /**
    * @brief Commits the curreent merge.
    *
    */
   void commit();
   /**
    * @brief This method removes all the handmade componentes before closing the merge view.
    *
    */
   void removeMergeComponents();
   /**
    * @brief When a conflict is marked as resolved the button is moved to the solved list. This action is triggered by a
    * ConflictButton.
    *
    */
   void onConflictResolved();
   /**
    * @brief Updates the current file diff view. This action is triggered by a ConflictButton.
    *
    */
   void onUpdateRequested();
};
