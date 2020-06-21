#include "FileDiffHighlighter.h"

#include <GitQlientStyles.h>
#include <QTextDocument>

FileDiffHighlighter::FileDiffHighlighter(QTextDocument *document)
   : QSyntaxHighlighter(document)
{
}

void FileDiffHighlighter::highlightBlock(const QString &text)
{
   setCurrentBlockState(previousBlockState() + 1);

   if (!text.isEmpty())
   {
      QTextCharFormat myFormat;

      switch (text.at(0).toLatin1())
      {
         case '@':
            myFormat.setForeground(GitQlientStyles::getOrange());
            myFormat.setFontWeight(QFont::ExtraBold);
            break;
         case '+':
            myFormat.setForeground(GitQlientStyles::getGreen());
            break;
         case '-':
            myFormat.setForeground(GitQlientStyles::getRed());
            break;
         default:
            break;
      }

      if (myFormat.isValid())
         setFormat(0, text.length(), myFormat);
   }
}
