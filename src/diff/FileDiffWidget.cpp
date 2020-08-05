#include "FileDiffWidget.h"

#include <GitBase.h>
#include <GitHistory.h>
#include <FileDiffView.h>
#include <CommitInfo.h>
#include <RevisionsCache.h>
#include <GitQlientSettings.h>
#include <CheckBox.h>
#include <FileEditor.h>
#include <GitLocal.h>

#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QScrollBar>
#include <QDateTime>
#include <QStackedWidget>
#include <QMessageBox>

FileDiffWidget::FileDiffWidget(const QSharedPointer<GitBase> &git, QSharedPointer<RevisionsCache> cache,
                               QWidget *parent)
   : IDiffWidget(git, cache, parent)
   , mBack(new QPushButton())
   , mGoPrevious(new QPushButton())
   , mGoNext(new QPushButton())
   , mEdition(new QPushButton())
   , mFullView(new QPushButton())
   , mSplitView(new QPushButton())
   , mSave(new QPushButton())
   , mStage(new QPushButton())
   , mRevert(new QPushButton())
   , mNewFile(new FileDiffView())
   , mOldFile(new FileDiffView())
   , mFileEditor(new FileEditor())
   , mViewStackedWidget(new QStackedWidget())
{
   mNewFile->setObjectName("newFile");
   mOldFile->setObjectName("oldFile");

   const auto optionsLayout = new QHBoxLayout();
   optionsLayout->setContentsMargins(10, 10, 0, 0);
   optionsLayout->setSpacing(10);
   optionsLayout->addWidget(mBack);
   optionsLayout->addWidget(mGoPrevious);
   optionsLayout->addWidget(mGoNext);
   optionsLayout->addWidget(mFullView);
   optionsLayout->addWidget(mSplitView);
   optionsLayout->addWidget(mEdition);
   optionsLayout->addWidget(mSave);
   optionsLayout->addWidget(mStage);
   optionsLayout->addWidget(mRevert);
   optionsLayout->addStretch();

   const auto diffLayout = new QHBoxLayout();
   diffLayout->setContentsMargins(QMargins());
   diffLayout->addWidget(mNewFile);
   diffLayout->addWidget(mOldFile);

   const auto diffFrame = new QFrame();
   diffFrame->setLayout(diffLayout);

   mViewStackedWidget->addWidget(diffFrame);
   mViewStackedWidget->addWidget(mFileEditor);

   const auto vLayout = new QVBoxLayout(this);
   vLayout->setContentsMargins(QMargins());
   vLayout->setSpacing(10);
   vLayout->addLayout(optionsLayout);
   vLayout->addWidget(mViewStackedWidget);

   GitQlientSettings settings;
   mFileVsFile
       = settings.localValue(mGit->getGitQlientSettingsDir(), GitQlientSettings::SplitFileDiffView, false).toBool();

   mBack->setIcon(QIcon(":/icons/back"));
   mBack->setToolTip(tr("Return to the view"));
   connect(mBack, &QPushButton::clicked, this, &FileDiffWidget::exitRequested);

   mGoPrevious->setIcon(QIcon(":/icons/arrow_up"));
   mGoPrevious->setToolTip(tr("Previous change"));
   connect(mGoPrevious, &QPushButton::clicked, this, &FileDiffWidget::moveChunkUp);

   mGoNext->setToolTip(tr("Next change"));
   mGoNext->setIcon(QIcon(":/icons/arrow_down"));
   connect(mGoNext, &QPushButton::clicked, this, &FileDiffWidget::moveChunkDown);

   mEdition->setIcon(QIcon(":/icons/edit"));
   mEdition->setCheckable(true);
   mEdition->setToolTip(tr("Edit file"));
   connect(mEdition, &QPushButton::toggled, this, &FileDiffWidget::enterEditionMode);

   mFullView->setIcon(QIcon(":/icons/text-file"));
   mFullView->setCheckable(true);
   mFullView->setToolTip(tr("Full file view"));
   connect(mFullView, &QPushButton::toggled, this, &FileDiffWidget::setFullViewEnabled);

   mSplitView->setIcon(QIcon(":/icons/split_view"));
   mSplitView->setCheckable(true);
   mSplitView->setToolTip(tr("Split file view"));
   connect(mSplitView, &QPushButton::toggled, this, &FileDiffWidget::setSplitViewEnabled);

   mSave->setIcon(QIcon(":/icons/save"));
   mSave->setDisabled(true);
   mSave->setToolTip(tr("Save"));
   connect(mSave, &QPushButton::clicked, mFileEditor, &FileEditor::saveFile);

   mStage->setIcon(QIcon(":/icons/staged"));
   mStage->setToolTip(tr("Stage file"));
   connect(mStage, &QPushButton::clicked, this, &FileDiffWidget::stageFile);

   mRevert->setIcon(QIcon(":/icons/close"));
   mRevert->setToolTip(tr("Revert changes"));
   connect(mRevert, &QPushButton::clicked, this, &FileDiffWidget::revertFile);

   mViewStackedWidget->setCurrentIndex(0);

   if (!mFileVsFile)
      mOldFile->setHidden(true);

   connect(mNewFile, &FileDiffView::signalScrollChanged, mOldFile, &FileDiffView::moveScrollBarToPos);
   connect(mOldFile, &FileDiffView::signalScrollChanged, mNewFile, &FileDiffView::moveScrollBarToPos);

   setAttribute(Qt::WA_DeleteOnClose);
}

void FileDiffWidget::clear()
{
   mNewFile->clear();
}

bool FileDiffWidget::reload()
{
   if (mCurrentSha == CommitInfo::ZERO_SHA)
      return configure(mCurrentSha, mPreviousSha, mCurrentFile, mEdition->isChecked());

   return false;
}

bool FileDiffWidget::configure(const QString &currentSha, const QString &previousSha, const QString &file,
                               bool editMode)
{
   const auto isWip = currentSha == CommitInfo::ZERO_SHA;
   mBack->setVisible(isWip);
   mEdition->setVisible(isWip);
   mSave->setVisible(isWip);
   mStage->setVisible(isWip);
   mRevert->setVisible(isWip);

   mCurrentFile = file;
   mCurrentSha = currentSha;
   mPreviousSha = previousSha;

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

      GitQlientSettings settings;
      mFileVsFile
          = settings.localValue(mGit->getGitQlientSettingsDir(), GitQlientSettings::SplitFileDiffView, false).toBool();

      if (editMode)
      {
         mEdition->setChecked(true);
         mSave->setEnabled(true);
      }
      else
      {
         mEdition->setChecked(false);
         mSave->setDisabled(true);
         mFullView->setChecked(!mFileVsFile);
         mSplitView->setChecked(mFileVsFile);
      }

      return true;
   }

   return false;
}

void FileDiffWidget::setSplitViewEnabled(bool enable)
{
   mFileVsFile = enable;

   mOldFile->setVisible(mFileVsFile);

   GitQlientSettings settings;
   settings.setLocalValue(mGit->getGitQlientSettingsDir(), GitQlientSettings::SplitFileDiffView, mFileVsFile);

   configure(mCurrentSha, mPreviousSha, mCurrentFile);

   mFullView->blockSignals(true);
   mFullView->setChecked(!mFileVsFile);
   mFullView->blockSignals(false);

   mGoNext->setEnabled(true);
   mGoPrevious->setEnabled(true);

   if (enable)
   {
      mSave->setDisabled(true);
      mEdition->blockSignals(true);
      mEdition->setChecked(false);
      mEdition->blockSignals(false);
      endEditFile();
   }
}

void FileDiffWidget::setFullViewEnabled(bool enable)
{
   mFileVsFile = !enable;

   mOldFile->setVisible(mFileVsFile);

   GitQlientSettings settings;
   settings.setLocalValue(mGit->getGitQlientSettingsDir(), GitQlientSettings::SplitFileDiffView, mFileVsFile);

   configure(mCurrentSha, mPreviousSha, mCurrentFile);

   mSplitView->blockSignals(true);
   mSplitView->setChecked(mFileVsFile);
   mSplitView->blockSignals(false);

   mGoNext->setDisabled(true);
   mGoPrevious->setDisabled(true);

   if (enable)
   {
      mSave->setDisabled(true);
      mEdition->blockSignals(true);
      mEdition->setChecked(false);
      mEdition->blockSignals(false);
      endEditFile();
   }
}

void FileDiffWidget::hideBackButton() const
{
   mBack->setVisible(true);
}

void FileDiffWidget::processDiff(const QString &text, QPair<QStringList, QVector<DiffInfo::ChunkInfo>> &newFileData,
                                 QPair<QStringList, QVector<DiffInfo::ChunkInfo>> &oldFileData)
{
   DiffInfo diff;
   // int newFileVariation = 0;
   int oldFileRow = 1;
   int newFileRow = 1;
   mChunks.clear();

   for (auto line : text.split("\n"))
   {
      if (mFileVsFile)
      {
         if (line.startsWith('-'))
         {
            line.remove(0, 1);

            if (diff.oldFile.startLine == -1)
               diff.oldFile.startLine = oldFileRow;

            oldFileData.first.append(line);

            ++oldFileRow;
         }
         else if (line.startsWith('+'))
         {
            line.remove(0, 1);

            if (diff.newFile.startLine == -1)
            {
               diff.newFile.startLine = newFileRow;
               diff.newFile.addition = true;
            }

            newFileData.first.append(line);

            ++newFileRow;
         }
         else
         {
            line.remove(0, 1);

            if (diff.oldFile.startLine != -1)
               diff.oldFile.endLine = oldFileRow - 1;

            if (diff.newFile.startLine != -1)
               diff.newFile.endLine = newFileRow - 1;

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

            ++oldFileRow;
            ++newFileRow;
         }
      }
      else
      {
         oldFileData.first.append(line);
         newFileData.first.append(line);
      }
   }
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

void FileDiffWidget::enterEditionMode(bool enter)
{
   if (enter)
   {
      mSave->setEnabled(true);
      mSplitView->blockSignals(true);
      mSplitView->setChecked(!enter);
      mSplitView->blockSignals(false);

      mFullView->blockSignals(true);
      mFullView->setChecked(!enter);
      mFullView->blockSignals(false);

      mFileEditor->editFile(mCurrentFile);
      mViewStackedWidget->setCurrentIndex(1);
   }
   else if (mFileVsFile)
      setSplitViewEnabled(true);
   else
      setFullViewEnabled(true);
}

void FileDiffWidget::endEditFile()
{
   mViewStackedWidget->setCurrentIndex(0);
}

void FileDiffWidget::stageFile()
{
   QScopedPointer<GitLocal> git(new GitLocal(mGit));
   const auto ret = git->markFileAsResolved(mCurrentFile);

   if (ret.success)
   {
      emit fileStaged(mCurrentFile);
      emit exitRequested();
   }
}

void FileDiffWidget::revertFile()
{
   const auto ret = QMessageBox::warning(
       this, tr("Revert all changes"),
       tr("Please, take into account that this will revert all the changes you have performed so far."),
       QMessageBox::Ok, QMessageBox::Cancel);

   if (ret == QMessageBox::Ok)
   {
      QScopedPointer<GitLocal> git(new GitLocal(mGit));
      const auto ret = git->checkoutFile(mCurrentFile);

      if (ret)
      {
         emit fileReverted(mCurrentFile);
         emit exitRequested();
      }
   }
}
