#pragma once

#include <QFrame>

class GitBase;
class QPushButton;

/**
 * @brief The ConflictButton class creates buttons that are used by the MergeWidget. The button is composed by three
 * different QPushButtons. The first one shows the file name and allows the user to change the selection when clicks
 * over it. Next to it there is the update button that allows the user to refresh the view in the merge widget. The last
 * button is the resolve button. This button allows the user to mark the merge conflict in the the file as solved and
 * adds the file to the commit.
 *
 */
class ConflictButton : public QFrame
{
   Q_OBJECT

signals:
   /**
    * @brief Signal triggered when the name is clicked.
    *
    * @param checked True if the button is selected, otherwise false.
    */
   void toggled(bool checked);
   /**
    * @brief Signal triggered when the user solves the merge conflict.
    *
    */
   void resolved();
   /**
    * @brief Signal triggered when the user requests an update of the file content.
    *
    */
   void updateRequested();

public:
   /**
    * @brief Default constructor.
    *
    * @param filename The file name.
    * @param inConflict Indicates if the file has conflicts.
    * @param git The git object to perform Git operations.
    * @param parent The parent wiget if needed.
    */
   explicit ConflictButton(const QString &filename, bool inConflict, const QSharedPointer<GitBase> &git,
                           QWidget *parent = nullptr);

   /**
    * @brief Sets the button as selected.
    *
    * @param checked The new check state.
    */
   void setChecked(bool checked);

private:
   QSharedPointer<GitBase> mGit;
   QString mFileName;
   QPushButton *mFile = nullptr;
   QPushButton *mResolve = nullptr;
   QPushButton *mUpdate = nullptr;

   /**
    * @brief Sets the button and the file as merge conflict.
    *
    * @param inConflict True if it is in conflict, otherwise false.
    */
   void setInConflict(bool inConflict);
   /**
    * @brief Resolves the conflict of the file and adds the file to the merge commit.
    *
    */
   void resolveConflict();
};
