#include "MergeWidget.h"

#include <GitBase.h>
#include <GitMerge.h>
#include <GitRemote.h>
#include <GitLocal.h>
#include <FileDiffWidget.h>
#include <CommitInfo.h>
#include <RevisionFiles.h>
#include <ConflictButton.h>

#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QStackedWidget>
#include <QFile>
#include <QMessageBox>

MergeWidget::MergeWidget(const QSharedPointer<RevisionsCache> &gitQlientCache, const QSharedPointer<GitBase> &git,
                         QWidget *parent)
   : QFrame(parent)
   , mGitQlientCache(gitQlientCache)
   , mGit(git)
   , mCenterStackedWidget(new QStackedWidget())
   , mCommitTitle(new QLineEdit())
   , mDescription(new QTextEdit())
   , mMergeBtn(new QPushButton(tr("Merge && Commit")))
   , mAbortBtn(new QPushButton(tr("Abort merge")))
{
   mCenterStackedWidget->setCurrentIndex(0);
   mCenterStackedWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

   const auto conflictBtnFrame = new QFrame();
   conflictBtnFrame->setObjectName("DiffButtonsFrame");

   mConflictBtnContainer = new QVBoxLayout(conflictBtnFrame);
   mConflictBtnContainer->setContentsMargins(QMargins());
   mConflictBtnContainer->setSpacing(5);
   mConflictBtnContainer->setAlignment(Qt::AlignTop);

   const auto conflictScrollArea = new QScrollArea();
   conflictScrollArea->setWidget(conflictBtnFrame);
   conflictScrollArea->setWidgetResizable(true);
   conflictScrollArea->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

   const auto autoMergedBtnFrame = new QFrame();
   autoMergedBtnFrame->setObjectName("DiffButtonsFrame");

   mAutoMergedBtnContainer = new QVBoxLayout(autoMergedBtnFrame);
   mAutoMergedBtnContainer->setContentsMargins(QMargins());
   mAutoMergedBtnContainer->setSpacing(5);
   mAutoMergedBtnContainer->setAlignment(Qt::AlignTop);

   const auto autoMergedScrollArea = new QScrollArea();
   autoMergedScrollArea->setWidget(autoMergedBtnFrame);
   autoMergedScrollArea->setWidgetResizable(true);
   autoMergedScrollArea->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

   mCommitTitle->setObjectName("leCommitTitle");

   mDescription->setMaximumHeight(125);
   mDescription->setPlaceholderText("Description");
   mDescription->setObjectName("teDescription");
   mDescription->setLineWrapMode(QTextEdit::WidgetWidth);
   mDescription->setReadOnly(false);
   mDescription->setAcceptRichText(false);

   mAbortBtn->setObjectName("Abort");
   mMergeBtn->setObjectName("Merge");

   const auto mergeBtnLayout = new QHBoxLayout();
   mergeBtnLayout->setContentsMargins(QMargins());
   mergeBtnLayout->addWidget(mAbortBtn);
   mergeBtnLayout->addStretch();
   mergeBtnLayout->addWidget(mMergeBtn);

   const auto mergeInfoLayout = new QVBoxLayout();
   mergeInfoLayout->setContentsMargins(QMargins());
   mergeInfoLayout->setSpacing(0);
   mergeInfoLayout->addWidget(mCommitTitle);
   mergeInfoLayout->addWidget(mDescription);
   mergeInfoLayout->addSpacerItem(new QSpacerItem(1, 10, QSizePolicy::Fixed, QSizePolicy::Fixed));
   mergeInfoLayout->addLayout(mergeBtnLayout);

   const auto mergeFrame = new QFrame();
   mergeFrame->setObjectName("mergeFrame");

   const auto conflictsLabel = new QLabel(tr("Files with conflicts"));
   conflictsLabel->setObjectName("FilesListTitle");

   const auto automergeLabel = new QLabel(tr("Merged files"));
   automergeLabel->setObjectName("FilesListTitle");

   const auto mergeLayout = new QVBoxLayout(mergeFrame);
   mergeLayout->setContentsMargins(QMargins());
   mergeLayout->setSpacing(0);
   mergeLayout->addWidget(conflictsLabel);
   mergeLayout->addWidget(conflictScrollArea);
   mergeLayout->addSpacerItem(new QSpacerItem(1, 10, QSizePolicy::Fixed, QSizePolicy::Fixed));
   mergeLayout->addWidget(automergeLabel);
   mergeLayout->addWidget(autoMergedScrollArea);
   mergeLayout->addSpacerItem(new QSpacerItem(1, 10, QSizePolicy::Fixed, QSizePolicy::Fixed));
   mergeLayout->addLayout(mergeInfoLayout);

   const auto layout = new QHBoxLayout(this);
   layout->setContentsMargins(QMargins());
   layout->addWidget(mergeFrame);
   layout->addWidget(mCenterStackedWidget);

   connect(mAbortBtn, &QPushButton::clicked, this, &MergeWidget::abort);
   connect(mMergeBtn, &QPushButton::clicked, this, &MergeWidget::commit);
}

void MergeWidget::configure(const RevisionFiles &files, ConflictReason reason)
{
   mReason = reason;

   QFile mergeMsg(mGit->getWorkingDir() + "/.git/MERGE_MSG");

   if (mergeMsg.open(QIODevice::ReadOnly))
   {
      const auto summary = QString::fromUtf8(mergeMsg.readLine()).trimmed();
      const auto description = QString::fromUtf8(mergeMsg.readAll()).trimmed();
      mCommitTitle->setText(summary);
      mDescription->setText(description);
      mergeMsg.close();
   }

   fillButtonFileList(files);
}

void MergeWidget::fillButtonFileList(const RevisionFiles &files)
{
   for (auto i = 0; i < files.count(); ++i)
   {
      const auto fileInConflict = files.statusCmp(i, RevisionFiles::CONFLICT);
      const auto fileName = files.getFile(i);
      const auto fileBtn = new ConflictButton(fileName, fileInConflict, mGit);
      fileBtn->setObjectName("FileBtn");

      connect(fileBtn, &ConflictButton::toggled, this, &MergeWidget::changeDiffView);
      connect(fileBtn, &ConflictButton::updateRequested, this, &MergeWidget::onUpdateRequested);
      connect(fileBtn, &ConflictButton::resolved, this, &MergeWidget::onConflictResolved);
      connect(fileBtn, &ConflictButton::signalEditFile, this, &MergeWidget::signalEditFile);

      const auto wip = mGitQlientCache->getCommitInfo(CommitInfo::ZERO_SHA);
      const auto fileDiffWidget = new FileDiffWidget(mGit);
      fileDiffWidget->configure(CommitInfo::ZERO_SHA, wip.parent(0), fileName);

      mConflictButtons.insert(fileBtn, fileDiffWidget);

      const auto index = mCenterStackedWidget->addWidget(fileDiffWidget);

      if (fileInConflict)
      {
         mConflictBtnContainer->addWidget(fileBtn);

         if (mCenterStackedWidget->count() == 0)
            mCenterStackedWidget->setCurrentIndex(index);
      }
      else
         mAutoMergedBtnContainer->addWidget(fileBtn);
   }
}

void MergeWidget::changeDiffView(bool fileBtnChecked)
{
   if (fileBtnChecked)
   {
      const auto end = mConflictButtons.constEnd();

      for (auto iter = mConflictButtons.constBegin(); iter != end; ++iter)
      {
         if (iter.key() != sender())
         {
            iter.key()->blockSignals(true);
            iter.key()->setChecked(false);
            iter.key()->blockSignals(false);
         }
         else
            mCenterStackedWidget->setCurrentWidget(iter.value());
      }
   }
}

void MergeWidget::abort()
{
   GitExecResult ret;

   switch (mReason)
   {
      case ConflictReason::Pull:
      case ConflictReason::Merge: {
         QScopedPointer<GitMerge> git(new GitMerge(mGit, mGitQlientCache));
         ret = git->abortMerge();
         break;
      }
      case ConflictReason::CherryPick: {
         QScopedPointer<GitLocal> git(new GitLocal(mGit));
         ret = git->cherryPickAbort();
         break;
      }
      default:
         break;
   }

   if (!ret.success)
      QMessageBox::warning(this, tr("Error aborting!"),
                           tr("The git command thrown an error: %1").arg(ret.output.toString()));
   else
   {
      removeMergeComponents();

      emit signalMergeFinished();
   }
}

void MergeWidget::commit()
{
   GitExecResult ret;

   switch (mReason)
   {
      case ConflictReason::Pull:
      case ConflictReason::Merge: {
         QScopedPointer<GitMerge> git(new GitMerge(mGit, mGitQlientCache));
         ret = git->applyMerge();
         break;
      }
      case ConflictReason::CherryPick: {
         QScopedPointer<GitLocal> git(new GitLocal(mGit));
         ret = git->cherryPickContinue();
         break;
      }
      default:
         break;
   }

   if (!ret.success)
      QMessageBox::warning(this, tr("Error merging!"),
                           tr("The git command throuwn an error: %1").arg(ret.output.toString()));
   else
   {
      removeMergeComponents();

      emit signalMergeFinished();
   }
}

void MergeWidget::removeMergeComponents()
{
   const auto end = mConflictButtons.constEnd();

   for (auto iter = mConflictButtons.constBegin(); iter != end; ++iter)
   {
      mCenterStackedWidget->removeWidget(iter.value());
      iter.value()->setParent(nullptr);
      delete iter.value();

      iter.key()->setParent(nullptr);
      delete iter.key();
   }

   mConflictButtons.clear();
   mCommitTitle->clear();
   mDescription->clear();
}

void MergeWidget::onConflictResolved()
{
   const auto conflictButton = qobject_cast<ConflictButton *>(sender());
   mAutoMergedBtnContainer->addWidget(conflictButton);

   mConflictButtons.value(conflictButton)->reload();
}

void MergeWidget::onUpdateRequested()
{
   const auto conflictButton = qobject_cast<ConflictButton *>(sender());
   mConflictButtons.value(conflictButton)->reload();
}
