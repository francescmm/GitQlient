/*
        Author: Marco Costalba (C) 2005-2007

        Copyright: See COPYING file that comes with this distribution

*/
#include "FullDiffWidget.h"
#include "common.h"
#include "domain.h"
#include "git.h"
#include "GitAsyncProcess.h"
#include <QScrollBar>
#include <QTextCharFormat>

void DiffHighlighter::highlightBlock(const QString &text)
{

   // state is used to count paragraphs, starting from 0
   setCurrentBlockState(previousBlockState() + 1);
   if (text.isEmpty())
      return;

   QBrush blue = QColor("#579BD5");
   QBrush green = QColor("#50FA7B");
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

FullDiffWidget::FullDiffWidget(QWidget *parent)
   : QTextEdit(parent)
{
   diffHighlighter = new DiffHighlighter(this);
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

void FullDiffWidget::scrollCursorToTop()
{

   QRect r = cursorRect();
   QScrollBar *vsb = verticalScrollBar();
   vsb->setValue(vsb->value() + r.top());
}

void FullDiffWidget::scrollLineToTop(int lineNum)
{

   QTextCursor tc = textCursor();
   tc.movePosition(QTextCursor::Start);
   tc.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor, lineNum);
   setTextCursor(tc);
   scrollCursorToTop();
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

void FullDiffWidget::centerOnFileHeader(StateInfo &st)
{

   if (st.fileName().isEmpty())
      return;

   target = st.fileName();
   bool combined = (st.isMerge() && !st.allMergeFiles());
   Git::getInstance()->formatPatchFileHeader(&target, st.sha(), st.diffToSha(), combined, st.allMergeFiles());
   seekTarget = !target.isEmpty();
   if (seekTarget)
      seekTarget = !centerTarget(target);
}

void FullDiffWidget::centerMatch(int id)
{

   if (matches.count() <= id)
      return;
   // FIXME
   //	patchTab->textEditDiff->setSelection(matches[id].paraFrom, matches[id].indexFrom,
   //	                                     matches[id].paraTo, matches[id].indexTo);
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
   if (!QGit::stripPartialParaghraps(fileChunk, &newLines, &halfLine))
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

void FullDiffWidget::update(StateInfo &st)
{

   bool combined = (st.isMerge() && !st.allMergeFiles());
   if (combined)
   {
      const Rev *r = Git::getInstance()->revLookup(st.sha());
      if (r)
         diffHighlighter->setCombinedLength(r->parentsCount());
   }
   else
      diffHighlighter->setCombinedLength(0);

   clear();

   Git::getInstance()->getDiff(st.sha(), this, st.diffToSha(), combined); // non blocking
}
