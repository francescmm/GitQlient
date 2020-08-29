#include "PrChangeListItem.h"

#include <DiffHelper.h>
#include <FileDiffView.h>
#include <LineNumberArea.h>

#include <QGridLayout>
#include <QLabel>

using namespace DiffHelper;

PrChangeListItem::PrChangeListItem(DiffChange change, QWidget *parent)
   : QFrame(parent)
   , mNewFileStartingLine(change.newFileStartLine)
   , mNewFileName(change.newFileName)
{
   setObjectName("PrChangeListItem");

   DiffHelper::processDiff(change.content, true, change.newData, change.oldData);

   mNewFileEndingLine = mNewFileStartingLine + change.newData.first.count();

   const auto oldFile = new FileDiffView();
   const auto numberArea = new LineNumberArea(oldFile, true);
   numberArea->setObjectName("LineNumberArea");
   oldFile->addNumberArea(numberArea);
   numberArea->setEditor(oldFile);
   oldFile->setStartingLine(change.oldFileStartLine - 1);
   oldFile->loadDiff(change.oldData.first.join("\n").trimmed(), change.oldData.second);
   oldFile->setMinimumWidth(535);
   oldFile->show();
   oldFile->setMinimumHeight(oldFile->getHeight());

   mNewFileDiff = new FileDiffView();
   mNewNumberArea = new LineNumberArea(mNewFileDiff, true);
   mNewNumberArea->setObjectName("LineNumberArea");
   mNewNumberArea->setEditor(mNewFileDiff);
   mNewFileDiff->setStartingLine(change.newFileStartLine - 1);
   mNewFileDiff->loadDiff(change.newData.first.join("\n").trimmed(), change.newData.second);
   mNewFileDiff->addNumberArea(mNewNumberArea);
   mNewFileDiff->setMinimumWidth(535);
   mNewFileDiff->show();
   mNewFileDiff->setMinimumHeight(mNewFileDiff->getHeight());

   connect(mNewNumberArea, &LineNumberArea::gotoReview, this, &PrChangeListItem::gotoReview);

   const auto fileName = change.oldFileName == change.newFileName
       ? change.newFileName
       : QString("%1 -> %2").arg(change.oldFileName, change.newFileName);

   const auto fileNameLabel = new QLabel(fileName);
   fileNameLabel->setObjectName("ChangeFileName");

   const auto headerLabel = new QLabel(change.header);
   headerLabel->setObjectName("ChangeHeader");

   const auto diffLayout = new QHBoxLayout();
   diffLayout->setContentsMargins(QMargins());
   diffLayout->setSpacing(5);
   diffLayout->addWidget(oldFile);
   diffLayout->addWidget(mNewFileDiff);

   const auto headerLayout = new QVBoxLayout();
   headerLayout->setContentsMargins(QMargins());
   headerLayout->addWidget(fileNameLabel);
   headerLayout->addWidget(headerLabel);

   const auto headerFrame = new QFrame();
   headerFrame->setObjectName("ChangeHeaderFrame");
   headerFrame->setLayout(headerLayout);

   const auto mainLayout = new QVBoxLayout(this);
   mainLayout->setContentsMargins(QMargins());
   mainLayout->setSpacing(0);
   mainLayout->addWidget(headerFrame);
   mainLayout->addLayout(diffLayout);
}

void PrChangeListItem::setBookmarks(const QMap<int, int> &bookmarks)
{
   mNewNumberArea->setCommentBookmarks(bookmarks);
}
