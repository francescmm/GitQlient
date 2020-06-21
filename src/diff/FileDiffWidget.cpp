#include "FileDiffWidget.h"

#include <GitHistory.h>
#include <FileDiffView.h>
#include <FileDiffHighlighter.h>
#include <CommitInfo.h>
#include <RevisionsCache.h>
#include <DiffInfoPanel.h>

#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QScrollBar>
#include <QDateTime>

FileDiffWidget::FileDiffWidget(const QSharedPointer<GitBase> &git, QSharedPointer<RevisionsCache> cache,
                               QWidget *parent)
   : QFrame(parent)
   , mGit(git)
   , mCache(cache)
   , mDiffView(new FileDiffView())
   , mGoPrevious(new QPushButton())
   , mGoNext(new QPushButton())
   , mDiffInfoPanel(new DiffInfoPanel(cache))

{
   setAttribute(Qt::WA_DeleteOnClose);

   mDiffHighlighter = new FileDiffHighlighter(mDiffView->document());

   mGoPrevious->setIcon(QIcon(":/icons/go_up"));
   mGoNext->setIcon(QIcon(":/icons/go_down"));

   const auto vLayout = new QVBoxLayout(this);
   vLayout->setContentsMargins(QMargins());
   vLayout->setSpacing(10);
   vLayout->addWidget(mDiffInfoPanel);
   vLayout->addWidget(mDiffView);
}

void FileDiffWidget::clear()
{
   mDiffView->clear();
}

bool FileDiffWidget::reload()
{
   if (mCurrentSha == CommitInfo::ZERO_SHA)
      return configure(mCurrentSha, mPreviousSha, mCurrentFile);

   return false;
}

bool FileDiffWidget::configure(const QString &currentSha, const QString &previousSha, const QString &file)
{
   mCurrentFile = file;
   mCurrentSha = currentSha;
   mPreviousSha = previousSha;

   mDiffInfoPanel->configure(currentSha, previousSha);

   auto destFile = file;

   if (destFile.contains("-->"))
      destFile = destFile.split("--> ").last().split("(").first().trimmed();

   QScopedPointer<GitHistory> git(new GitHistory(mGit));
   auto text = git->getFileDiff(currentSha == CommitInfo::ZERO_SHA ? QString() : currentSha, previousSha, destFile);

   auto pos = 0;
   for (auto i = 0; i < 5; ++i)
      pos = text.indexOf("\n", pos + 1);

   text = text.mid(pos + 1);

   if (!text.isEmpty())
   {
      const auto pos = mDiffView->verticalScrollBar()->value();
      auto cursor = mDiffView->textCursor();
      const auto tmpCursor = mDiffView->textCursor().position();
      mDiffView->setPlainText(text);

      mRowIndex = 0;

      cursor.setPosition(tmpCursor);
      mDiffView->setTextCursor(cursor);

      mDiffView->verticalScrollBar()->setValue(pos);

      return true;
   }

   return false;
}
