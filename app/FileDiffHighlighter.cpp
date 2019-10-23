#include "FileDiffHighlighter.h"

FileDiffHighlighter::FileDiffHighlighter(QTextDocument *document)
   : QSyntaxHighlighter(document)
{
}

void FileDiffHighlighter::highlightBlock(const QString &text)
{
   setCurrentBlockState(previousBlockState() + 1);

   if (!text.isEmpty())
   {
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
         default:
            break;
      }

      if (myFormat.isValid())
         setFormat(0, text.length(), myFormat);
   }
}

void FileDiffHighlighter::resetState()
{
   mFirstModificationFound = false;
}
