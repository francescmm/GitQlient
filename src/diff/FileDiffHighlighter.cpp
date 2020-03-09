#include "FileDiffHighlighter.h"

#include <GitQlientStyles.h>

FileDiffHighlighter::FileDiffHighlighter(QTextDocument *document)
   : QSyntaxHighlighter(document)
{
}

void FileDiffHighlighter::highlightBlock(const QString &text)
{
   setCurrentBlockState(previousBlockState() + 1);

   if (!text.isEmpty())
   {
      QBrush green = GitQlientStyles::getGreen();
      QBrush magenta = GitQlientStyles::getRed();
      QBrush orange = GitQlientStyles::getOrange();

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
