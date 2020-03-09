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

#include <QPlainTextEdit>

/*!
 \brief The FileDiffView is an overload QPlainTextEdit class used to show the contents of a file diff between two
 commits.

*/
class FileDiffView : public QPlainTextEdit
{
   Q_OBJECT

public:
   /*!
    \brief Default constructor.

    \param parent The parent widget if needed.
   */
   FileDiffView(QWidget *parent = nullptr);

protected:
   /*!
    \brief Overloaded method to process the resize event. Used to set an updated geometry to the line number area.

    \param event The resize event.
   */
   void resizeEvent(QResizeEvent *event) override;

private:
   /*!
    \brief Updates the line number area width based on the number of the line.

    \param newBlockCount The number of the line.
   */
   void updateLineNumberAreaWidth(int newBlockCount);
   /*!
    \brief Updates the line number area whenever the QPlainTextEdit emits the updateRequest signal.

    \param rect The rect area that was updated.
    \param dy The increment.
   */
   void updateLineNumberArea(const QRect &rect, int dy);

   /*!
    \brief Method called by the line number area to paint the content of the QPlainTextEdit.

    \param event The paint event.
    */
   void lineNumberAreaPaintEvent(QPaintEvent *event);

   /*!
    \brief Returns the width of the line number area.

    \return int The width in pixels.
    */
   int lineNumberAreaWidth();

private:
   class LineNumberArea : public QWidget
   {
   public:
      LineNumberArea(FileDiffView *editor);

      QSize sizeHint() const override;

   protected:
      void paintEvent(QPaintEvent *event) override;

   private:
      FileDiffView *fileDiffWidget;
   };

   LineNumberArea *mLineNumberArea;
};
