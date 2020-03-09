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

class QPushButton;

/*!
 \brief The DiffButton is a special implementation of QFrame that has a similar behaviour to a QPushButton. The main
 difference is that DiffButton allows the user to emit a close signal that also closes the button. So it's self
 destructive.

Since it's a speialization for the Diff widget, the DiffButton contains in first place an icon that indicates if the
button refers to a commit or a single file. Then it shows the file name (or just a label indicating that is a commit
diff) followed by the two SHAs that are part of the diff. At the end of the button there is a close button that destroys
the widget and triggers a signal to close the diff view.

 \class DiffButton DiffButton.h "DiffButton.h"
*/
class DiffButton : public QFrame
{
   Q_OBJECT

signals:
   /*!
    \brief Signal triggered when the user clicks the DiffButton.
   */
   void clicked();

public:
   /*!
    \brief Default constructor.

    \param text The text that will be shown in the button.
    \param icon The icon that represents the diff: file or commit.
    \param parent The parent widget if needed.
   */
   explicit DiffButton(const QString &text, const QString &icon, QWidget *parent = nullptr);
   /*!
    \brief Sets the button as it is selected.
   */
   void setSelected();
   /*!
    \brief Unsets the select status of the button.
   */
   void setUnselected();

protected:
   /*!
    \brief Overrided function to process the press event. Used to trigger the click signal and change the style of the
    button. \param e The event
   */
   void mousePressEvent(QMouseEvent *e) override;

private:
   bool mPressed = false;
   QPushButton *mCloseBtn = nullptr;
};
