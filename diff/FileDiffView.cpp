#include "FileDiffView.h"

#include <GitQlientStyles.h>

#include <QPainter>
#include <QTextBlock>

FileDiffView::FileDiffView(QWidget *parent)
   : QPlainTextEdit(parent)
   , mLineNumberArea(new LineNumberArea(this))
{
   setReadOnly(true);

   connect(this, &FileDiffView::blockCountChanged, this, &FileDiffView::updateLineNumberAreaWidth);
   connect(this, &FileDiffView::updateRequest, this, &FileDiffView::updateLineNumberArea);

   updateLineNumberAreaWidth(0);
}

int FileDiffView::lineNumberAreaWidth()
{
   auto digits = 1;
   auto max = std::max(1, blockCount());

   while (max >= 10)
   {
      max /= 10;
      ++digits;
   }

   return 8 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void FileDiffView::updateLineNumberAreaWidth(int /* newBlockCount */)
{
   setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void FileDiffView::updateLineNumberArea(const QRect &rect, int dy)
{
   if (dy != 0)
      mLineNumberArea->scroll(0, dy);
   else
      mLineNumberArea->update(0, rect.y(), mLineNumberArea->width(), rect.height());

   if (rect.contains(viewport()->rect()))
      updateLineNumberAreaWidth(0);
}

void FileDiffView::resizeEvent(QResizeEvent *e)
{
   QPlainTextEdit::resizeEvent(e);

   const auto cr = contentsRect();

   mLineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void FileDiffView::lineNumberAreaPaintEvent(QPaintEvent *event)
{
   QPainter painter(mLineNumberArea);
   painter.fillRect(event->rect(), QColor("#202122"));

   auto block = firstVisibleBlock();
   auto blockNumber = block.blockNumber();
   auto top = blockBoundingGeometry(block).translated(contentOffset()).top();
   auto bottom = top + blockBoundingRect(block).height();

   while (block.isValid() && top <= event->rect().bottom())
   {
      if (block.isVisible() && bottom >= event->rect().top())
      {
         const auto number = QString::number(blockNumber + 1);
         painter.setPen(GitQlientStyles::getTextColor());
         painter.drawText(0, static_cast<int>(top), mLineNumberArea->width() - 3, fontMetrics().height(),
                          Qt::AlignRight, number);
      }

      block = block.next();
      top = bottom;
      bottom = top + blockBoundingRect(block).height();
      ++blockNumber;
   }
}

LineNumberArea::LineNumberArea(FileDiffView *editor)
   : QWidget(editor)
{
   fileDiffWidget = editor;
}

QSize LineNumberArea::sizeHint() const
{
   return { fileDiffWidget->lineNumberAreaWidth(), 0 };
}

void LineNumberArea::paintEvent(QPaintEvent *event)
{
   fileDiffWidget->lineNumberAreaPaintEvent(event);
}
