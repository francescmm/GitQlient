#include "FileDiffWidget.h"

#include <GitHistory.h>
#include <FileDiffView.h>
#include <CommitInfo.h>
#include <RevisionsCache.h>
#include <DiffInfoPanel.h>
#include <GitQlientSettings.h>

#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QScrollBar>
#include <QDateTime>
#include <QCheckBox>

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
   , mFileVsFileCheck(new QCheckBox(tr("Show file vs file")))
   , mGoTop(new QPushButton())
   , mGoUp(new QPushButton())
   , mGoDown(new QPushButton())
   , mGoBottom(new QPushButton())
   , mNavFrame(new QFrame())

{
   GitQlientSettings settings;
   mFileVsFile = settings.value("FileVsFile", false).toBool();
   mFileVsFileCheck->setChecked(mFileVsFile);

   mNewFile->setObjectName("newFile");
   mOldFile->setObjectName("oldFile");

   setAttribute(Qt::WA_DeleteOnClose);

   mGoPrevious->setIcon(QIcon(":/icons/go_up"));
   mGoNext->setIcon(QIcon(":/icons/go_down"));

   const auto optionsLayout = new QHBoxLayout();
   optionsLayout->setContentsMargins(QMargins());
   optionsLayout->setSpacing(10);
   optionsLayout->addWidget(mFileVsFileCheck);

   mGoTop->setIcon(QIcon(":/icons/arrow_up_full"));
   connect(mGoTop, &QPushButton::clicked, this, &FileDiffWidget::moveTop);

   mGoUp->setIcon(QIcon(":/icons/arrow_up"));
   connect(mGoUp, &QPushButton::clicked, this, &FileDiffWidget::moveChunkUp);

   mGoDown->setIcon(QIcon(":/icons/arrow_down"));
   connect(mGoDown, &QPushButton::clicked, this, &FileDiffWidget::moveChunkDown);

   mGoBottom->setIcon(QIcon(":/icons/arrow_down_full"));
   connect(mGoBottom, &QPushButton::clicked, this, &FileDiffWidget::moveBottomChunk);

   const auto navLayout = new QVBoxLayout(mNavFrame);
   navLayout->setContentsMargins(QMargins());
   navLayout->setSpacing(10);
   navLayout->addWidget(mGoTop);
   navLayout->addWidget(mGoUp);
   navLayout->addStretch();
   navLayout->addWidget(mGoDown);
   navLayout->addWidget(mGoBottom);

   mNavFrame->setVisible(mFileVsFile);

   const auto diffLayout = new QHBoxLayout();
   diffLayout->setContentsMargins(QMargins());
   diffLayout->addWidget(mNavFrame);
   diffLayout->addWidget(mNewFile);
   diffLayout->addWidget(mOldFile);

   const auto vLayout = new QVBoxLayout(this);
   vLayout->setContentsMargins(QMargins());
   vLayout->setSpacing(10);
   vLayout->addWidget(mDiffInfoPanel);
   vLayout->addLayout(optionsLayout);
   vLayout->addLayout(diffLayout);

   connect(mNewFile, &FileDiffView::signalScrollChanged, mOldFile, &FileDiffView::moveScrollBarToPos);
   connect(mOldFile, &FileDiffView::signalScrollChanged, mNewFile, &FileDiffView::moveScrollBarToPos);
   connect(mFileVsFileCheck, &QCheckBox::toggled, this, &FileDiffWidget::setFileVsFileEnable);

   mOldFile->setVisible(mFileVsFile);
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
      QPair<QStringList, QVector<DiffInfo::ChunkInfo>> oldData;
      QPair<QStringList, QVector<DiffInfo::ChunkInfo>> newData;

      processDiff(text, newData, oldData);

      mOldFile->blockSignals(true);
      mOldFile->loadDiff(oldData.first.join('\n'), oldData.second);
      mOldFile->blockSignals(false);

      mNewFile->blockSignals(true);
      mNewFile->loadDiff(newData.first.join('\n'), newData.second);
      mNewFile->blockSignals(false);

      return true;
   }

   return false;
}

void FileDiffWidget::setFileVsFileEnable(bool enable)
{
   mFileVsFile = enable;

   mOldFile->setVisible(mFileVsFile);
   mNavFrame->setVisible(mFileVsFile);

   GitQlientSettings settings;
   settings.setValue("FileVsFile", mFileVsFile);

   configure(mCurrentSha, mPreviousSha, mCurrentFile);
}

void FileDiffWidget::editMode(const QString &)
{

}

void FileDiffWidget::processDiff(const QString &text, QPair<QStringList, QVector<DiffInfo::ChunkInfo>> &newFileData,
                                 QPair<QStringList, QVector<DiffInfo::ChunkInfo>> &oldFileData)
{
   auto row = 1;
   DiffInfo diff;
   mChunks.clear();

   for (auto line : text.split("\n"))
   {
      if (mFileVsFile)
      {
         if (line.startsWith('-'))
         {
            line.remove(0, 1);

            if (diff.oldFile.startLine == -1)
               diff.oldFile.startLine = row;

            oldFileData.first.append(line);

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

            newFileData.first.append(line);
         }
         else
         {
            line.remove(0, 1);

            if (diff.oldFile.startLine != -1)
               diff.oldFile.endLine = row;

            if (diff.newFile.startLine != -1)
               diff.newFile.endLine = row - 1;

            if (diff.isValid())
            {
               if (diff.newFile.isValid())
               {
                  newFileData.second.append(diff.newFile);
                  mChunks.append(diff.newFile);
               }

               if (diff.oldFile.isValid())
               {
                  oldFileData.second.append(diff.oldFile);
                  mChunks.append(diff.oldFile);
               }
            }

            oldFileData.first.append(line);
            newFileData.first.append(line);

            diff = DiffInfo();
         }

         ++row;
      }
      else
      {
         oldFileData.first.append(line);
         newFileData.first.append(line);
      }
   }
}

void FileDiffWidget::moveTop()
{
   mCurrentChunkLine = 0;

   mNewFile->moveScrollBarToPos(mCurrentChunkLine);
   mOldFile->moveScrollBarToPos(mCurrentChunkLine);
}

void FileDiffWidget::moveChunkUp()
{
   for (auto i = mChunks.count() - 1; i >= 0; --i)
   {
      if (auto chunkStart = mChunks.at(i).startLine; chunkStart < mCurrentChunkLine)
      {
         mCurrentChunkLine = chunkStart;

         mNewFile->moveScrollBarToPos(mCurrentChunkLine - 1);
         mOldFile->moveScrollBarToPos(mCurrentChunkLine - 1);

         break;
      }
   }
}

void FileDiffWidget::moveChunkDown()
{
   const auto endIter = mChunks.constEnd();
   auto iter = mChunks.constBegin();

   for (; iter != endIter; ++iter)
      if (iter->startLine > mCurrentChunkLine)
         break;

   if (iter != endIter)
   {
      mCurrentChunkLine = iter->startLine;

      mNewFile->moveScrollBarToPos(mCurrentChunkLine - 1);
      mOldFile->moveScrollBarToPos(mCurrentChunkLine - 1);
   }
}

void FileDiffWidget::moveBottomChunk()
{
   mCurrentChunkLine = mNewFile->blockCount();

   mNewFile->moveScrollBarToPos(mNewFile->blockCount());
   mOldFile->moveScrollBarToPos(mOldFile->blockCount());
}
