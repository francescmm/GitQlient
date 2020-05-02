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
   void accept() override;
   /*!
    \brief Copies the global settings into the local ones.

    \param checkState The check state.
   */
   void copyGlobalSettings(int checkState);
};
