#include "FileDiffView.h"

#include <QPainter>
#include <QTextBlock>

FileDiffView::FileDiffView(QWidget *parent)
   : QPlainTextEdit(parent)
{
   setReadOnly(true);

   lineNumberArea = new LineNumberArea(this);

   connect(this, &FileDiffView::blockCountChanged, this, &FileDiffView::updateLineNumberAreaWidth);
   connect(this, &FileDiffView::updateRequest, this, &FileDiffView::updateLineNumberArea);

   updateLineNumberAreaWidth(0);
}

int FileDiffView::lineNumberAreaWidth()
{
   int digits = 1;
   int max = qMax(1, blockCount());
   while (max >= 10)
   {
      max /= 10;
      ++digits;
   }

   int space = 8 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;

   return space;
}

void FileDiffView::updateLineNumberAreaWidth(int /* newBlockCount */)
{
   setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void FileDiffView::updateLineNumberArea(const QRect &rect, int dy)
{
   if (dy)
      lineNumberArea->scroll(0, dy);
   else
      lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

   if (rect.contains(viewport()->rect()))
      updateLineNumberAreaWidth(0);
}

void FileDiffView::resizeEvent(QResizeEvent *e)
{
   QPlainTextEdit::resizeEvent(e);

   QRect cr = contentsRect();
   lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void FileDiffView::lineNumberAreaPaintEvent(QPaintEvent *event)
{
   QPainter painter(lineNumberArea);
   painter.fillRect(event->rect(), QColor("#202122"));

   QTextBlock block = firstVisibleBlock();
   int blockNumber = block.blockNumber();
   auto top = blockBoundingGeometry(block).translated(contentOffset()).top();
   auto bottom = top + blockBoundingRect(block).height();

   while (block.isValid() && top <= event->rect().bottom())
   {
      if (block.isVisible() && bottom >= event->rect().top())
      {
         QString number = QString::number(blockNumber + 1);
         painter.setPen(Qt::white);
         painter.drawText(0, static_cast<int>(top), lineNumberArea->width() - 3, fontMetrics().height(), Qt::AlignRight,
                          number);
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
   return QSize(fileDiffWidget->lineNumberAreaWidth(), 0);
}

void LineNumberArea::paintEvent(QPaintEvent *event)
{
   fileDiffWidget->lineNumberAreaPaintEvent(event);
}
