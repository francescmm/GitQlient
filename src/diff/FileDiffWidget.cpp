#include "FileDiffWidget.h"

#include <ButtonLink.hpp>
#include <CheckBox.h>
#include <CommitInfo.h>
#include <DiffHelper.h>
#include <FileDiffView.h>
#include <FileEditor.h>
#include <GitBase.h>
#include <GitCache.h>
#include <GitHistory.h>
#include <GitLocal.h>
#include <GitPatches.h>
#include <GitQlientSettings.h>
#include <HunkWidget.h>
#include <LineNumberArea.h>

#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QStackedWidget>
#include <QTemporaryFile>
#include <QToolTip>

FileDiffWidget::FileDiffWidget(const QSharedPointer<GitBase> &git, QSharedPointer<GitCache> cache, QWidget *parent)
   : IDiffWidget(git, cache, parent)
   , mBack(new QPushButton())
   , mGoPrevious(new QPushButton())
   , mGoNext(new QPushButton())
   , mEdition(new QPushButton())
   , mHunksView(new QPushButton())
   , mFullView(new QPushButton())
   , mSplitView(new QPushButton())
   , mSave(new QPushButton())
   , mStage(new QPushButton())
   , mRevert(new QPushButton())
   , mFileNameLabel(new ButtonLink())
   , mTitleFrame(new QFrame())
   , mUnifiedFile(new FileDiffView())
   , mNewFile(new FileDiffView())
   , mSearchOld(new QLineEdit())
   , mOldFile(new FileDiffView())
   , mFileEditor(new FileEditor())
   , mHunksLayout(new QVBoxLayout())
   , mHunksFrame(new QFrame())
   , mHunkSpacer(new QSpacerItem(1, 1, QSizePolicy::Fixed, QSizePolicy::Expanding))
   , mViewStackedWidget(new QStackedWidget())
{
   mCurrentSha = ZERO_SHA;

   mUnifiedFile->addNumberArea(new LineNumberArea(mUnifiedFile, false, true));

   mNewFile->addNumberArea(new LineNumberArea(mNewFile));
   mOldFile->addNumberArea(new LineNumberArea(mOldFile));

   mNewFile->setObjectName("newFile");
   mOldFile->setObjectName("oldFile");

   GitQlientSettings settings(mGit->getGitDir());
   QFont font = mNewFile->font();
   const auto points = settings.globalValue("FileDiffView/FontSize", 8).toInt();
   font.setPointSize(points);
   mUnifiedFile->setFont(font);
   mNewFile->setFont(font);
   mOldFile->setFont(font);

   const auto optionsLayout = new QHBoxLayout();
   optionsLayout->setContentsMargins(5, 5, 0, 0);
   optionsLayout->setSpacing(5);
   optionsLayout->addWidget(mBack);
   optionsLayout->addWidget(mGoPrevious);
   optionsLayout->addWidget(mGoNext);
   optionsLayout->addWidget(mHunksView);
   optionsLayout->addWidget(mFullView);
   optionsLayout->addWidget(mSplitView);
   optionsLayout->addWidget(mEdition);
   optionsLayout->addWidget(mSave);
   optionsLayout->addWidget(mStage);
   optionsLayout->addWidget(mRevert);
   optionsLayout->addStretch();

   const auto searchUnified = new QLineEdit();
   searchUnified->setObjectName("SearchInput");
   searchUnified->setPlaceholderText(tr("Press Enter to search a text... "));
   connect(searchUnified, &QLineEdit::editingFinished, this,
           [this, searchUnified]() { DiffHelper::findString(searchUnified->text(), mUnifiedFile, this); });

   const auto unifiedLayout = new QVBoxLayout();
   unifiedLayout->setContentsMargins(QMargins());
   unifiedLayout->setSpacing(5);
   unifiedLayout->addWidget(searchUnified);
   unifiedLayout->addWidget(mUnifiedFile);

   const auto unifiedDiffLayout = new QHBoxLayout();
   unifiedDiffLayout->setContentsMargins(10, 0, 10, 0);
   unifiedDiffLayout->addLayout(unifiedLayout);

   const auto unifiedDiffFrame = new QFrame();
   unifiedDiffFrame->setLayout(unifiedDiffLayout);

   const auto searchNew = new QLineEdit();
   searchNew->setObjectName("SearchInput");
   searchNew->setPlaceholderText(tr("Press Enter to search a text... "));
   connect(searchNew, &QLineEdit::editingFinished, this,
           [this, searchNew]() { DiffHelper::findString(searchNew->text(), mNewFile, this); });

   const auto newFileLayout = new QVBoxLayout();
   newFileLayout->setContentsMargins(QMargins());
   newFileLayout->setSpacing(5);
   newFileLayout->addWidget(searchNew);
   newFileLayout->addWidget(mNewFile);

   mSearchOld->setPlaceholderText(tr("Press Enter to search a text... "));
   mSearchOld->setObjectName("SearchInput");
   connect(mSearchOld, &QLineEdit::editingFinished, this,
           [this]() { DiffHelper::findString(mSearchOld->text(), mOldFile, this); });

   const auto oldFileLayout = new QVBoxLayout();
   oldFileLayout->setContentsMargins(QMargins());
   oldFileLayout->setSpacing(5);
   oldFileLayout->addWidget(mSearchOld);
   oldFileLayout->addWidget(mOldFile);

   const auto splitDiffLayout = new QHBoxLayout();
   splitDiffLayout->setContentsMargins(10, 0, 10, 0);
   splitDiffLayout->addLayout(newFileLayout);
   splitDiffLayout->addLayout(oldFileLayout);

   const auto splitDiffFrame = new QFrame();
   splitDiffFrame->setLayout(splitDiffLayout);

   mHunksLayout->setContentsMargins(QMargins());
   mHunksLayout->setAlignment(Qt::AlignTop | Qt::AlignVCenter);
   mHunksFrame->setLayout(mHunksLayout);
   mHunksFrame->setObjectName("HunksFrame");

   const auto scrollArea = new QScrollArea();
   scrollArea->setWidget(mHunksFrame);
   scrollArea->setWidgetResizable(true);

   mViewStackedWidget->addWidget(scrollArea);
   mViewStackedWidget->addWidget(unifiedDiffFrame);
   mViewStackedWidget->addWidget(splitDiffFrame);
   mViewStackedWidget->addWidget(mFileEditor);

   const auto titleLayout = new QHBoxLayout(mTitleFrame);
   titleLayout->setContentsMargins(0, 10, 0, 10);
   titleLayout->setSpacing(0);
   titleLayout->addStretch();
   titleLayout->addWidget(mFileNameLabel);
   titleLayout->addStretch();

   const auto vLayout = new QVBoxLayout(this);
   vLayout->setContentsMargins(QMargins());
   vLayout->setSpacing(5);
   vLayout->addWidget(mTitleFrame);
   vLayout->addLayout(optionsLayout);
   vLayout->addWidget(mViewStackedWidget);

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

   mHunksView->setIcon(QIcon(":/icons/commit-list"));
   mHunksView->setCheckable(true);
   mHunksView->setToolTip(tr("Hunks view"));
   connect(mHunksView, &QPushButton::toggled, this, &FileDiffWidget::setHunksViewEnabled);

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
   connect(mSave, &QPushButton::clicked, mEdition, &QPushButton::toggle);

   mStage->setIcon(QIcon(":/icons/staged"));
   mStage->setToolTip(tr("Stage file"));
   connect(mStage, &QPushButton::clicked, this, &FileDiffWidget::stageFile);

   mRevert->setIcon(QIcon(":/icons/close"));
   mRevert->setToolTip(tr("Revert changes"));
   connect(mRevert, &QPushButton::clicked, this, &FileDiffWidget::revertFile);

   mViewStackedWidget->setCurrentIndex(settings.globalValue("DefaultDiffView", false).toInt());

   connect(mFileNameLabel, &ButtonLink::clicked, this, [this]() {
      QApplication::clipboard()->setText(mFileNameLabel->text());
      const auto button = qobject_cast<ButtonLink *>(sender());
      QToolTip::showText(QCursor::pos(), tr("Copied!"), button);
   });
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
   return setup(mCurrentFile, mIsCached, mEdition->isChecked(), mCurrentSha, mPreviousSha);
}

void FileDiffWidget::updateFontSize()
{
   GitQlientSettings settings;
   const auto fontSize = settings.globalValue("FileDiffView/FontSize", 8).toInt();

   auto font = mNewFile->font();
   font.setPointSize(fontSize);

   auto cursor = mNewFile->textCursor();

   mUnifiedFile->selectAll();
   mUnifiedFile->setFont(font);
   mUnifiedFile->setTextCursor(cursor);

   mNewFile->selectAll();
   mNewFile->setFont(font);
   mNewFile->setTextCursor(cursor);

   cursor = mOldFile->textCursor();
   mOldFile->selectAll();
   mOldFile->setFont(font);
   mOldFile->setTextCursor(cursor);
}

void FileDiffWidget::hideHunks() const
{
   mHunksView->setHidden(true);
}

bool FileDiffWidget::setup(const QString &file, bool isCached, bool editMode, QString currentSha, QString previousSha)
{
   if (previousSha.isEmpty())
   {
      QScopedPointer<GitHistory> git(new GitHistory(mGit));
      previousSha = mCache->commitInfo(ZERO_SHA).firstParent();
   }

   if (configure(file, isCached, currentSha, previousSha))
   {
      if (editMode)
      {
         mEdition->setChecked(true);
         mSave->setEnabled(true);
      }
      else if (mCurrentSha != ZERO_SHA)
      {
         mBack->setHidden(true);
         mEdition->setHidden(true);
         mSave->setHidden(true);
         mStage->setHidden(true);
         mRevert->setHidden(true);
      }
      else
      {
         mEdition->setChecked(false);
         mSave->setDisabled(true);
         mHunksView->blockSignals(true);
         mHunksView->setChecked(mViewStackedWidget->currentIndex() == View::Hunks);
         mHunksView->blockSignals(false);
         mFullView->blockSignals(true);
         mFullView->setChecked(mViewStackedWidget->currentIndex() == View::Unified && !mHunksView->isChecked());
         mFullView->blockSignals(false);
         mSplitView->blockSignals(true);
         mSplitView->setChecked(mViewStackedWidget->currentIndex() == View::Split && !mHunksView->isChecked());
         mSplitView->blockSignals(false);
      }

      return true;
   }

   return false;
}

bool FileDiffWidget::configure(const QString &file, bool isCached, QString currentSha, QString previousSha)
{
   if (currentSha.isEmpty())
      currentSha = mCurrentSha;

   if (previousSha.isEmpty())
      previousSha = mPreviousSha;

   auto destFile = file;

   if (destFile.contains("-->"))
      destFile = destFile.split("--> ").last().split("(").first().trimmed();

   QString text;
   QScopedPointer<GitHistory> git(new GitHistory(mGit));

   if (const auto ret
       = git->getFullFileDiff(currentSha == ZERO_SHA ? QString() : currentSha, previousSha, destFile, isCached);
       ret.success)
   {
      text = ret.output;

      if (text.isEmpty())
      {
         if (const auto ret = git->getUntrackedFileDiff(destFile); ret.success)
            text = ret.output;
      }

      if (text.startsWith("* "))
         return false;
   }

   mFileNameLabel->setText(file);

   mIsCached = isCached;
   mCurrentFile = file;
   mCurrentSha = currentSha;
   mPreviousSha = previousSha;

   auto pos = 0;
   for (auto i = 0; i < 5; ++i)
      pos = text.indexOf("\n", pos + 1);

   text = text.mid(pos + 1);

   if (!text.isEmpty())
   {
      processHunks(destFile);

      if (mViewStackedWidget->currentIndex() == View::Split)
      {
         QPair<QStringList, QVector<ChunkDiffInfo::ChunkInfo>> newData;
         QPair<QStringList, QVector<ChunkDiffInfo::ChunkInfo>> oldData;
         mChunks = DiffHelper::processDiff(text, newData, oldData);

         mOldFile->blockSignals(true);
         mOldFile->loadDiff(oldData.first.join('\n'), oldData.second);
         mOldFile->blockSignals(false);

         mNewFile->blockSignals(true);
         mNewFile->loadDiff(newData.first.join('\n'), newData.second);
         mNewFile->blockSignals(false);
      }
      else
      {
         const auto data = DiffHelper::processDiff(text);

         mUnifiedFile->blockSignals(true);
         mUnifiedFile->loadDiff(text, data);
         mUnifiedFile->blockSignals(false);
      }

      return true;
   }

   return false;
}

void FileDiffWidget::setSplitViewEnabled(bool enable)
{
   configure(mCurrentFile, mIsCached, mCurrentSha, mPreviousSha);

   mViewStackedWidget->setCurrentIndex(View::Split);

   mFullView->blockSignals(true);
   mFullView->setChecked(false);
   mFullView->blockSignals(false);

   mHunksView->blockSignals(true);
   mHunksView->setChecked(false);
   mHunksView->blockSignals(false);

   mGoNext->setEnabled(true);
   mGoPrevious->setEnabled(true);

   if (enable)
   {
      mSave->setDisabled(true);
      mEdition->blockSignals(true);
      mEdition->setChecked(false);
      mEdition->blockSignals(false);
   }
}

void FileDiffWidget::setFullViewEnabled(bool enable)
{
   configure(mCurrentFile, mIsCached, mCurrentSha, mPreviousSha);

   mViewStackedWidget->setCurrentIndex(View::Unified);

   mSplitView->blockSignals(true);
   mSplitView->setChecked(false);
   mSplitView->blockSignals(false);

   mHunksView->blockSignals(true);
   mHunksView->setChecked(false);
   mHunksView->blockSignals(false);

   mGoNext->setDisabled(true);
   mGoPrevious->setDisabled(true);

   if (enable)
   {
      mSave->setDisabled(true);
      mEdition->blockSignals(true);
      mEdition->setChecked(false);
      mEdition->blockSignals(false);
   }
}

void FileDiffWidget::setHunksViewEnabled(bool enable)
{
   mSave->setDisabled(true);

   mSplitView->blockSignals(true);
   mSplitView->setChecked(!enable);
   mSplitView->blockSignals(false);

   mFullView->blockSignals(true);
   mFullView->setChecked(!enable);
   mFullView->blockSignals(false);

   mViewStackedWidget->setCurrentIndex(View::Hunks);
}

void FileDiffWidget::hideBackButton() const
{
   mBack->setVisible(true);
}

void FileDiffWidget::moveChunkUp()
{
   for (auto i = mChunks.chunks.count() - 1; i >= 0; --i)
   {
      auto &chunk = mChunks.chunks.at(i);

      if (auto [chunkNewStart, chunkOldStart] = std::make_tuple(chunk.newFile.startLine, chunk.oldFile.startLine);
          chunkNewStart < mCurrentChunkLine || chunkOldStart < mCurrentChunkLine)
      {
         if (chunkNewStart < mCurrentChunkLine)
            mCurrentChunkLine = chunkNewStart;
         else if (chunkOldStart < mCurrentChunkLine)
            mCurrentChunkLine = chunkOldStart;

         mNewFile->moveScrollBarToPos(mCurrentChunkLine - 1);
         mOldFile->moveScrollBarToPos(mCurrentChunkLine - 1);

         break;
      }
   }
}

void FileDiffWidget::moveChunkDown()
{
   const auto endIter = mChunks.chunks.constEnd();
   auto iter = mChunks.chunks.constBegin();

   for (; iter != endIter; ++iter)
   {
      if (iter->newFile.startLine > mCurrentChunkLine)
      {
         mCurrentChunkLine = iter->newFile.startLine;
         break;
      }
      else if (iter->oldFile.startLine > mCurrentChunkLine)
      {
         mCurrentChunkLine = iter->oldFile.startLine;
         break;
      }
   }

   if (iter != endIter)
   {
      mNewFile->moveScrollBarToPos(mCurrentChunkLine - 1);
      mOldFile->moveScrollBarToPos(mCurrentChunkLine - 1);
   }
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

      mHunksView->blockSignals(true);
      mHunksView->setChecked(false);
      mHunksView->blockSignals(false);

      mFileEditor->editFile(mCurrentFile);

      mViewStackedWidget->setCurrentIndex(View::Edition);
   }
   else if (mViewStackedWidget->currentIndex() == View::Split)
      setSplitViewEnabled(true);
   else if (mViewStackedWidget->currentIndex() == View::Unified)
      setFullViewEnabled(true);
   else if (mViewStackedWidget->currentIndex() == View::Hunks)
      setHunksViewEnabled(true);
}

void FileDiffWidget::endEditFile()
{
   enterEditionMode(false);
}

void FileDiffWidget::stageFile()
{
   QScopedPointer<GitLocal> git(new GitLocal(mGit));
   const auto ret = git->stageFile(mCurrentFile);

   if (ret.success)
   {
      mIsCached = true;

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

void FileDiffWidget::processHunks(const QString &file)
{
   QScopedPointer<GitHistory> git(new GitHistory(mGit));

   GitExecResult hunks;
   if (mCurrentSha == ZERO_SHA)
      hunks = git->getWipFileDiff(file, mIsCached);
   else
      hunks = git->getFileDiff(file, mIsCached, mCurrentSha, mPreviousSha);

   for (auto hunk : std::as_const(mHunks))
      delete hunk;

   mHunks.clear();
   mHunksLayout->removeItem(mHunkSpacer);

   if (hunks.success && !hunks.output.isEmpty())
   {
      auto chunk = hunks.output;
      const auto header = chunk.left(chunk.indexOf("@@"));
      chunk.remove(0, header.length());

      auto chunkLines = chunk.split('\n');
      QString hunk;

      for (auto &line : chunkLines)
      {
         if (line.startsWith("@@") && !hunk.isEmpty())
         {
            if (hunk.endsWith('\n'))
               hunk = hunk.left(hunk.size() - 1);

            createAndAddHunk(file, header, hunk);
            hunk.clear();
         }

         hunk.append(line);

         if (!(line.length() == 1 && line[0] == '\n'))
            hunk.append('\n');
      }

      if (!hunk.isEmpty())
      {
         // This time we need to remove two \n characters since output brings one by default.
         if (hunk.endsWith('\n'))
            hunk = hunk.left(hunk.size() - 2);

         createAndAddHunk(file, header, hunk);
      }

      if (!mHunks.isEmpty())
         mHunksLayout->addItem(mHunkSpacer);
   }

   mHunksView->setEnabled(hunks.success && !mHunks.isEmpty());
}

void FileDiffWidget::createAndAddHunk(const QString &file, const QString &header, const QString &hunk)
{
   auto hunkView = new HunkWidget(mGit, mCache, file, header, hunk, mIsCached, mCurrentSha == ZERO_SHA);
   connect(hunkView, &HunkWidget::hunkStaged, this, &FileDiffWidget::deleteHunkView);

   mHunksLayout->addWidget(hunkView);
   mHunks.append(hunkView);
}

void FileDiffWidget::deleteHunkView()
{
   auto hunkView = qobject_cast<HunkWidget *>(sender());

   const auto iter = std::find(mHunks.begin(), mHunks.end(), hunkView);

   if (iter != mHunks.end())
      mHunks.erase(iter);

   hunkView->deleteLater();

   if (mHunks.isEmpty() && !mIsCached)
   {
      // We stage the file no matter what: if the file has no modifications, nothing will happen. But if the file has
      // modifications this will force Git to refresh the information about the changes and avoid partially cached
      // misleading info.

      QScopedPointer<GitLocal> gitLocal(new GitLocal(mGit));
      gitLocal->stageFile(mCurrentFile);

      emit exitRequested();
   }
   else
      reload();
}
