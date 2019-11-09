#include "FullDiffWidget.h"

#include <Revision.h>
#include <GitAsyncProcess.h>
#include <git.h>

#include <QScrollBar>
#include <QTextCharFormat>
#include <QTextCodec>

namespace
{
bool stripPartialParaghraps(const QByteArray &ba, QString *dst, QString *prev)
{

   QTextCodec *tc = QTextCodec::codecForLocale();

   if (ba.endsWith('\n'))
   { // optimize common case
      *dst = tc->toUnicode(ba);

      // handle rare case of a '\0' inside content
      while (dst->size() < ba.size() && ba.at(dst->size()) == '\0')
      {
         QString s = tc->toUnicode(ba.mid(dst->size() + 1)); // sizes should match
         dst->append(" ").append(s);
      }

      dst->truncate(dst->size() - 1); // strip trailing '\n'
      if (!prev->isEmpty())
      {
         dst->prepend(*prev);
         prev->clear();
      }
      return true;
   }
   QString src = tc->toUnicode(ba);
   // handle rare case of a '\0' inside content
   while (src.size() < ba.size() && ba.at(src.size()) == '\0')
   {
      QString s = tc->toUnicode(ba.mid(src.size() + 1));
      src.append(" ").append(s);
   }

   int idx = src.lastIndexOf('\n');
   if (idx == -1)
   {
      prev->append(src);
      dst->clear();
      return false;
   }
   *dst = src.left(idx).prepend(*prev); // strip trailing '\n'
   *prev = src.mid(idx + 1); // src[idx] is '\n', skip it
   return true;
}
}

void DiffHighlighter::highlightBlock(const QString &text)
{

   // state is used to count paragraphs, starting from 0
   setCurrentBlockState(previousBlockState() + 1);
   if (text.isEmpty())
      return;

   QBrush blue = QColor("#579BD5");
   QBrush green = QColor("#8DC944");
   QBrush magenta = QColor("#FF5555");
   QBrush orange = QColor("#FFB86C");

   QTextCharFormat myFormat;
   const char firstChar = text.at(0).toLatin1();
   switch (firstChar)
   {
      case '@':
         myFormat.setForeground(orange);
         myFormat.setFontWeight(QFont::ExtraBold);
         break;
      case '+':
         myFormat.setForeground(green);
         break;
      case '-':
         myFormat.setForeground(magenta);
         break;
      case 'c':
      case 'd':
      case 'i':
      case 'n':
      case 'o':
      case 'r':
      case 's':
         if (text.startsWith("diff --git a/"))
         {
            myFormat.setForeground(blue);
            myFormat.setFontWeight(QFont::ExtraBold);
         }
         else if (text.startsWith("copy ") || text.startsWith("index ") || text.startsWith("new ")
                  || text.startsWith("old ") || text.startsWith("rename ") || text.startsWith("similarity "))
            myFormat.setForeground(blue);

         else if (cl > 0 && text.startsWith("diff --combined"))
         {
            myFormat.setForeground(blue);
            myFormat.setFontWeight(QFont::ExtraBold);
         }
         break;
      case ' ':
         if (cl > 0)
         {
            if (text.left(cl).contains('+'))
               myFormat.setForeground(green);
            else if (text.left(cl).contains('-'))
               myFormat.setForeground(magenta);
         }
         break;
   }
   if (myFormat.isValid())
      setFormat(0, text.length(), myFormat);

   FullDiffWidget *pc = static_cast<FullDiffWidget *>(parent());
   if (pc->matches.count() > 0)
   {
      int indexFrom, indexTo;
      if (pc->getMatch(currentBlockState(), &indexFrom, &indexTo))
      {

         QTextEdit *te = dynamic_cast<QTextEdit *>(parent());
         QTextCharFormat fmt;
         fmt.setFont(te->currentFont());
         fmt.setFontWeight(QFont::Bold);
         fmt.setForeground(blue);
         if (indexTo == 0)
            indexTo = text.length();

         setFormat(indexFrom, indexTo - indexFrom, fmt);
      }
   }
}

FullDiffWidget::FullDiffWidget(QSharedPointer<Git> git, QWidget *parent)
   : QTextEdit(parent)
   , mGit(git)
{
   diffHighlighter = new DiffHighlighter(this);

   QFont font;
   font.setFamily(QString::fromUtf8("Ubuntu Mono"));
   setFont(font);
   setObjectName("textEditDiff");
   setUndoRedoEnabled(false);
   setLineWrapMode(QTextEdit::NoWrap);
   setReadOnly(true);
   setTextInteractionFlags(Qt::TextSelectableByMouse);
}

void FullDiffWidget::clear()
{
   QTextEdit::clear();
   patchRowData.clear();
   halfLine = "";
   matches.clear();
   diffLoaded = false;
   seekTarget = !target.isEmpty();
}

void FullDiffWidget::refresh()
{

   int topPara = topToLineNum();
   setUpdatesEnabled(false);
   QByteArray tmp(patchRowData);
   clear();
   patchRowData = tmp;
   processData(patchRowData, &topPara);
   scrollLineToTop(topPara);
   setUpdatesEnabled(true);
}

void FullDiffWidget::scrollLineToTop(int lineNum)
{

   QTextCursor tc = textCursor();
   tc.movePosition(QTextCursor::Start);
   tc.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor, lineNum);
   setTextCursor(tc);
}

int FullDiffWidget::positionToLineNum(int pos)
{

   QTextCursor tc = textCursor();
   tc.setPosition(pos);
   return tc.blockNumber();
}

int FullDiffWidget::topToLineNum()
{

   return cursorForPosition(QPoint(1, 1)).blockNumber();
}

bool FullDiffWidget::centerTarget(const QString &target)
{

   moveCursor(QTextCursor::Start);

   // find() updates cursor position
   if (!find(target, QTextDocument::FindCaseSensitively | QTextDocument::FindWholeWords))
      return false;

   // move to the beginning of the line
   moveCursor(QTextCursor::StartOfLine);

   // grap copy of current cursor state
   QTextCursor tc = textCursor();

   // move the target line to the top
   moveCursor(QTextCursor::End);
   setTextCursor(tc);

   return true;
}

void FullDiffWidget::procReadyRead(const QByteArray &data)
{

   patchRowData.append(data);
   if (document()->isEmpty() && isVisible())
      processData(data);
}

void FullDiffWidget::typeWriterFontChanged()
{
   setPlainText(toPlainText());
}

void FullDiffWidget::processData(const QByteArray &fileChunk, int *prevLineNum)
{

   QString newLines;
   if (!stripPartialParaghraps(fileChunk, &newLines, &halfLine))
      return;

   if (!prevLineNum && curFilter == VIEW_ALL)
      goto skip_filter; // optimize common case

   { // scoped code because of goto

      QString filteredLines;
      int notNegCnt = 0, notPosCnt = 0;
      QVector<int> toAdded(1), toRemoved(1); // lines count from 1

      // prevLineNum will be set to the number of corresponding
      // line in full patch. Number is negative just for algorithm
      // reasons, prevLineNum counts lines from 1
      if (prevLineNum && prevFilter == VIEW_ALL)
         *prevLineNum = -(*prevLineNum); // set once

      const QStringList sl(newLines.split('\n', QString::KeepEmptyParts));
      for (auto it : sl)
      {

         // do not remove diff header because of centerTarget
         bool n = it.startsWith('-') && !it.startsWith("---");
         bool p = it.startsWith('+') && !it.startsWith("+++");

         if (!p)
            notPosCnt++;
         if (!n)
            notNegCnt++;

         toAdded.append(notNegCnt);
         toRemoved.append(notPosCnt);

         int curLineNum = toAdded.count() - 1;

         bool toRemove = (n && curFilter == VIEW_ADDED) || (p && curFilter == VIEW_REMOVED);
         if (!toRemove)
            filteredLines.append(it).append('\n');

         if (prevLineNum && *prevLineNum == notNegCnt && prevFilter == VIEW_ADDED)
            *prevLineNum = -curLineNum; // set once

         if (prevLineNum && *prevLineNum == notPosCnt && prevFilter == VIEW_REMOVED)
            *prevLineNum = -curLineNum; // set once
      }
      if (prevLineNum && *prevLineNum <= 0)
      {
         if (curFilter == VIEW_ALL)
            *prevLineNum = -(*prevLineNum);

         else if (curFilter == VIEW_ADDED)
            *prevLineNum = toAdded.at(-(*prevLineNum));

         else if (curFilter == VIEW_REMOVED)
            *prevLineNum = toRemoved.at(-(*prevLineNum));

         if (*prevLineNum < 0)
            *prevLineNum = 0;
      }
      newLines = filteredLines;

   } // end of scoped code

skip_filter:

   setUpdatesEnabled(false);

   if (prevLineNum || document()->isEmpty())
   { // use the faster setPlainText()

      setPlainText(newLines);
      moveCursor(QTextCursor::Start);
   }
   else
   {
      int topLine = cursorForPosition(QPoint(1, 1)).blockNumber();
      append(newLines);
      if (topLine > 0)
         scrollLineToTop(topLine);
   }
   QScrollBar *vsb = verticalScrollBar();
   vsb->setValue(vsb->value() + cursorRect().top());
   setUpdatesEnabled(true);
}

void FullDiffWidget::procFinished()
{

   if (!patchRowData.endsWith("\n"))
      patchRowData.append('\n'); // flush pending half lines

   refresh(); // show patchRowData content

   if (seekTarget)
      seekTarget = !centerTarget(target);

   diffLoaded = true;
}

void FullDiffWidget::loadDiff(const QString &sha, const QString &diffToSha)
{
   diffHighlighter->setCombinedLength(0);

   clear();

   mGit->getDiff(sha, this, diffToSha, false); // non blocking
}

bool FullDiffWidget::getMatch(int para, int *indexFrom, int *indexTo)
{

   for (int i = 0; i < matches.count(); i++)
      if (matches[i].paraFrom <= para && matches[i].paraTo >= para)
      {

         *indexFrom = (para == matches[i].paraFrom ? matches[i].indexFrom : 0);
         *indexTo = (para == matches[i].paraTo ? matches[i].indexTo : 0);
         return true;
      }
   return false;
}
