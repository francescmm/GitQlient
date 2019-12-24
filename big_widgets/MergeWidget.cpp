#include "MergeWidget.h"

#include <git.h>
#include <FileDiffWidget.h>
#include <CommitInfo.h>

#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QStackedWidget>
#include <QFile>

MergeWidget::MergeWidget(const QSharedPointer<Git> git, QWidget *parent)
   : QFrame(parent)
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

   connect(mAbortBtn, &QPushButton::clicked, this, []() {});
   connect(mMergeBtn, &QPushButton::clicked, this, []() {});
}

void MergeWidget::configure()
{
   const auto files = mGit->getWipFiles();

   QFile mergeMsg(mGit->getGitDir() + "/MERGE_MSG");

   if (mergeMsg.open(QIODevice::ReadOnly))
   {
      const auto summary = mergeMsg.readLine().trimmed();
      const auto description = mergeMsg.readAll().trimmed();
      mCommitTitle->setText(summary);
      mDescription->setText(description);
      mergeMsg.close();
   }

   fillButtonFileList(files);
}

void MergeWidget::fillButtonFileList(const RevisionFile &files)
{
   for (auto i = 0; i < files.count(); ++i)
   {
      const auto fileName = mGit->filePath(files, i);
      const auto fileBtn = new QPushButton(fileName);
      fileBtn->setObjectName("FileBtn");
      fileBtn->setCheckable(true);

      connect(fileBtn, &QPushButton::toggled, this, &MergeWidget::changeDiffView);

      const auto fileDiffWidget = new FileDiffWidget(mGit);
      fileDiffWidget->configure(ZERO_SHA, mGit->getCommitInfo(ZERO_SHA).parent(0), fileName);

      mConflictButtons.insert(fileBtn, fileDiffWidget);

      const auto index = mCenterStackedWidget->addWidget(fileDiffWidget);

      if (files.statusCmp(i, RevisionFile::CONFLICT))
      {
         mConflictBtnContainer->addWidget(fileBtn);

         fileBtn->setChecked(true);

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
