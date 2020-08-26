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
   , mNewFileDiff(new FileDiffView())
{
   setObjectName("PrChangeListItem");

   DiffHelper::processDiff(change.content, true, change.newData, change.oldData);

   const auto oldFile = new FileDiffView();
   const auto numberArea = new LineNumberArea(oldFile, true);
   oldFile->addNumberArea(numberArea);
   numberArea->setEditor(oldFile);
   oldFile->show();
   oldFile->setMinimumHeight(oldFile->getHeight());
   oldFile->setStartingLine(change.oldFileStartLine - 1);
   oldFile->loadDiff(change.oldData.first.join("\n"), change.oldData.second);

   mNewNumberArea = new LineNumberArea(mNewFileDiff, true);
   mNewNumberArea->setEditor(mNewFileDiff);
   mNewFileDiff->addNumberArea(mNewNumberArea);
   mNewFileDiff->show();
   mNewFileDiff->setMinimumHeight(oldFile->getHeight());
   mNewFileDiff->setStartingLine(change.newFileStartLine - 1);
   mNewFileDiff->loadDiff(change.newData.first.join("\n"), change.newData.second);

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
