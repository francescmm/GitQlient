#include "FullDiffWidget.h"

#include <CommitInfo.h>
#include <git.h>

#include <QScrollBar>
#include <QTextCharFormat>
#include <QTextCodec>

DiffHighlighter::DiffHighlighter(QTextEdit *p)
   : QSyntaxHighlighter(p)
{
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
         break;
      default:
         break;
   }
   if (myFormat.isValid())
      setFormat(0, text.length(), myFormat);
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

void FullDiffWidget::processData(const QString &fileChunk)
{
   if (mPreviousDiffText != fileChunk)
   {
      mPreviousDiffText = fileChunk;
      auto pos = verticalScrollBar()->value();

      setUpdatesEnabled(false);

      clear();

      setPlainText(fileChunk);
      moveCursor(QTextCursor::Start);

      verticalScrollBar()->setValue(pos);

      setUpdatesEnabled(true);
   }
}

void FullDiffWidget::loadDiff(const QString &sha, const QString &diffToSha)
{
   const auto ret = mGit->getCommitDiff(sha, diffToSha);

   if (ret.success)
      processData(ret.output.toString());
}
