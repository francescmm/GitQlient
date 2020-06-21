#include "FileDiffWidget.h"

#include <GitHistory.h>
#include <FileDiffView.h>
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
   , mNewFile(new FileDiffView())
   , mOldFile(new FileDiffView())
   , mGoPrevious(new QPushButton())
   , mGoNext(new QPushButton())
   , mDiffInfoPanel(new DiffInfoPanel(cache))

{
   mNewFile->setObjectName("newFile");
   mOldFile->setObjectName("oldFile");

   setAttribute(Qt::WA_DeleteOnClose);

   mGoPrevious->setIcon(QIcon(":/icons/go_up"));
   mGoNext->setIcon(QIcon(":/icons/go_down"));

   const auto diffLayout = new QHBoxLayout();
   diffLayout->addWidget(mNewFile);
   diffLayout->addWidget(mOldFile);

   const auto vLayout = new QVBoxLayout(this);
   vLayout->setContentsMargins(QMargins());
   vLayout->setSpacing(10);
   vLayout->addWidget(mDiffInfoPanel);
   vLayout->addLayout(diffLayout);

   connect(mNewFile, &FileDiffView::signalScrollChanged, mOldFile, &FileDiffView::moveScrollBarToPos);
   connect(mOldFile, &FileDiffView::signalScrollChanged, mNewFile, &FileDiffView::moveScrollBarToPos);
}

void FileDiffWidget::clear()
{
   mNewFile->clear();
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
      QString oldText;
      QString newText;
      auto row = 1;

      fileDiffs.clear();

      DiffInfo diff;
      QVector<DiffInfo::ChunkInfo> newFileDiffs;
      QVector<DiffInfo::ChunkInfo> oldFileDiffs;

      for (auto line : text.split("\n"))
      {
         if (line.startsWith('-'))
         {
            line.remove(0, 1);

            if (diff.oldFile.startLine == -1)
               diff.oldFile.startLine = row;

            oldText.append(line).append('\n');

            --row;
         }
         else if (line.startsWith('+'))
         {
            line.remove(0, 1);

            if (diff.newFile.startLine == -1)
            {
               diff.newFile.startLine = row;
               diff.newFile.addition = true;
            }

            newText.append(line).append('\n');
         }
         else
         {
            line.remove(0, 1);

            if (diff.oldFile.startLine != -1)
               diff.oldFile.endLine = row - 1;

            if (diff.newFile.startLine != -1)
               diff.newFile.endLine = row - 1;

            if (diff.isValid())
            {
               fileDiffs.append(diff);

               if (diff.newFile.isValid())
                  newFileDiffs.append(diff.newFile);

               if (diff.oldFile.isValid())
                  oldFileDiffs.append(diff.oldFile);
            }

            diff = DiffInfo();

            oldText.append(line).append('\n');
            newText.append(line).append('\n');
         }

         ++row;
      }

      mOldFile->blockSignals(true);
      mOldFile->loadDiff(oldText, oldFileDiffs);
      mOldFile->blockSignals(false);

      mNewFile->blockSignals(true);
      mNewFile->loadDiff(newText, newFileDiffs);
      mNewFile->blockSignals(false);

      return true;
   }

   return false;
}
