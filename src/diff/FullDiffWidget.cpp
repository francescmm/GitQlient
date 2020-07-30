#include "FullDiffWidget.h"

#include <CommitInfo.h>
#include <GitHistory.h>
#include <RevisionsCache.h>
#include <GitQlientStyles.h>

#include <QScrollBar>
#include <QTextCharFormat>
#include <QTextCodec>
#include <QVBoxLayout>

FullDiffWidget::DiffHighlighter::DiffHighlighter(QTextEdit *p)
   : QSyntaxHighlighter(p)
{
}

void FullDiffWidget::DiffHighlighter::highlightBlock(const QString &text)
{
   // state is used to count paragraphs, starting from 0
   setCurrentBlockState(previousBlockState() + 1);
   if (text.isEmpty())
      return;

   QTextCharFormat myFormat;
   const char firstChar = text.at(0).toLatin1();
   switch (firstChar)
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
      case 'c':
      case 'd':
      case 'i':
      case 'n':
      case 'o':
      case 'r':
      case 's':
         if (text.startsWith("diff --git a/"))
         {
            myFormat.setForeground(GitQlientStyles::getBlue());
            myFormat.setFontWeight(QFont::ExtraBold);
         }
         else if (text.startsWith("copy ") || text.startsWith("index ") || text.startsWith("new ")
                  || text.startsWith("old ") || text.startsWith("rename ") || text.startsWith("similarity "))
            myFormat.setForeground(GitQlientStyles::getBlue());
         break;
      default:
         break;
   }
   if (myFormat.isValid())
      setFormat(0, text.length(), myFormat);
}

FullDiffWidget::FullDiffWidget(const QSharedPointer<GitBase> &git, QSharedPointer<RevisionsCache> cache,
                               QWidget *parent)
   : IDiffWidget(git, cache, parent)
   , mDiffWidget(new QTextEdit())
{
   setAttribute(Qt::WA_DeleteOnClose);

   diffHighlighter = new DiffHighlighter(mDiffWidget);

   QFont font;
   font.setFamily(QString::fromUtf8("Ubuntu Mono"));
   mDiffWidget->setFont(font);
   mDiffWidget->setObjectName("textEditDiff");
   mDiffWidget->setUndoRedoEnabled(false);
   mDiffWidget->setLineWrapMode(QTextEdit::NoWrap);
   mDiffWidget->setReadOnly(true);
   mDiffWidget->setTextInteractionFlags(Qt::TextSelectableByMouse);

   const auto layout = new QVBoxLayout(this);
   layout->setContentsMargins(QMargins());
   layout->setSpacing(10);
   layout->addWidget(mDiffWidget);
}

bool FullDiffWidget::reload()
{
   if (mCurrentSha != CommitInfo::ZERO_SHA)
   {
      QScopedPointer<GitHistory> git(new GitHistory(mGit));
      const auto ret = git->getCommitDiff(mCurrentSha, mPreviousSha);

      if (ret.success && !ret.output.toString().isEmpty())
      {
         loadDiff(mCurrentSha, mPreviousSha, ret.output.toString());
         return true;
      }
   }

   return false;
}

void FullDiffWidget::processData(const QString &fileChunk)
{
   if (mPreviousDiffText != fileChunk)
   {
      mPreviousDiffText = fileChunk;

      const auto pos = mDiffWidget->verticalScrollBar()->value();

      mDiffWidget->setUpdatesEnabled(false);
      mDiffWidget->clear();
      mDiffWidget->setPlainText(fileChunk);
      mDiffWidget->moveCursor(QTextCursor::Start);
      mDiffWidget->verticalScrollBar()->setValue(pos);
      mDiffWidget->setUpdatesEnabled(true);
   }
}

void FullDiffWidget::loadDiff(const QString &sha, const QString &diffToSha, const QString &diffData)
{
   mCurrentSha = sha;
   mPreviousSha = diffToSha;

   processData(diffData);
}
