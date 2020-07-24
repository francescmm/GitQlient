#pragma once

/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2020 Francesc M.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QPlainTextEdit>
#include <DiffInfo.h>

class FileDiffHighlighter;

/*!
 \brief The FileDiffView is an overload QPlainTextEdit class used to show the contents of a file diff between two
 commits.

*/
class FileDiffView : public QPlainTextEdit
{
   Q_OBJECT

signals:
   /**
    * @brief signalScrollChanged Signal emited when the scrollbar changes its position.
    * @param value The new scrollbar position.
    */
   void signalScrollChanged(int value);

public:
   /*!
    \brief Default constructor.

    \param parent The parent widget if needed.
   */
   explicit FileDiffView(QWidget *parent = nullptr);

   /**
    * @brief Default constructor
    */
   ~FileDiffView();

   /**
    * @brief loadDiff Loads the text edit based on a diff text.
    * @param text The text representing a diff
    * @return True if correctly loaded, otherwise false.
    */
   void loadDiff(QString text, const QVector<DiffInfo::ChunkInfo> &fileDiffInfo);

   /**
    * @brief moveScrollBarToPos Moves the vertical scroll bar to the value defined in @p value.
    * @param value The new scroll bar value.
    */
   void moveScrollBarToPos(int value);

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

   LineNumberArea *mLineNumberArea = nullptr;
   FileDiffHighlighter *mDiffHighlighter = nullptr;
};
