#pragma once

#include <QDialog>

namespace Ui
{
class GitConfigDlg;
}

class GitBase;

/*!
 \brief The GitConfigDlg allows the user to configure the local and global user and email.

*/
class GitConfigDlg : public QDialog
{
   Q_OBJECT

public:
   /*!
    \brief Default constructor.

    \param git The git object to perform Git operations.
    \param parent The parent widget if needed.
   */
   explicit GitConfigDlg(const QSharedPointer<GitBase> &git, QWidget *parent = nullptr);
   /*!
    \brief Destructor.

   */
   ~GitConfigDlg() override;

private:
   Ui::GitConfigDlg *ui;
   QSharedPointer<GitBase> mGit;

   /*!
    \brief Validates the data input by the user and stores it if correct.

   */
   void accepted();
   /*!
    \brief Copies the global settings into the local ones.

    \param checkState The check state.
   */
   void copyGlobalSettings(int checkState);
};
