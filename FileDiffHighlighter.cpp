#include "FileDiffHighlighter.h"

FileDiffHighlighter::FileDiffHighlighter(QTextDocument *document)
   : QSyntaxHighlighter(document)
{
}

void FileDiffHighlighter::highlightBlock(const QString &text)
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
   const auto row = currentBlockState();

   switch (firstChar)
   {
      case '@':
         myFormat.setForeground(orange);
         myFormat.setFontWeight(QFont::ExtraBold);
         break;
      case '+':
         myFormat.setForeground(green);
         mModifiedRows.append(row);
         break;
      case '-':
         mModifiedRows.append(row);
         myFormat.setForeground(magenta);
         break;
      default:
         break;
   }

   if (myFormat.isValid())
      setFormat(0, text.length(), myFormat);
}

void FileDiffHighlighter::resetState()
{
   mFirstModificationFound = false;
   mModifiedRows.clear();
}
