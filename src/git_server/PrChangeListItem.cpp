#include "PrChangeListItem.h"

#include <DiffHelper.h>
#include <FileDiffView.h>

#include <QGridLayout>
#include <QLabel>

using namespace DiffHelper;

PrChangeListItem::PrChangeListItem(DiffChange change, QWidget *parent)
   : QFrame(parent)
{
   setObjectName("PrChangeListItem");

   DiffHelper::processDiff(change.content, true, change.newData, change.oldData);

   const auto oldFile = new FileDiffView();
   oldFile->show();
   oldFile->setMinimumHeight(oldFile->getHeight());
   oldFile->setStartingLine(change.oldFileStartLine - 1);
   oldFile->loadDiff(change.oldData.first.join("\n"), change.oldData.second);

   const auto newFile = new FileDiffView();
   newFile->show();
   newFile->setMinimumHeight(oldFile->getHeight());
   newFile->setStartingLine(change.newFileStartLine - 1);
   newFile->loadDiff(change.newData.first.join("\n"), change.newData.second);

   const auto oFileName = new QLabel(change.oldFileName);
   oFileName->setObjectName("ChangeFileName");

   const auto nFileName = new QLabel(change.newFileName);
   nFileName->setObjectName("ChangeFileName");

   const auto mainLayout = new QGridLayout(this);
   mainLayout->setContentsMargins(QMargins());
   mainLayout->setSpacing(0);
   mainLayout->addWidget(oFileName, 0, 0);
   mainLayout->addWidget(nFileName, 0, 1);
   mainLayout->addWidget(oldFile, 1, 0);
   mainLayout->addWidget(newFile, 1, 1);
}
