#include "CommitDiffWidget.h"

#include <FileListWidget.h>
#include <GitCache.h>

#include <QVBoxLayout>
#include <QLabel>

CommitDiffWidget::CommitDiffWidget(QSharedPointer<GitBase> git, QSharedPointer<GitCache> cache, QWidget *parent)
   : QFrame(parent)
   , mGit(git)
   , mCache(cache)
   , fileListWidget(new FileListWidget(mGit, cache))
{
   setAttribute(Qt::WA_DeleteOnClose);

   QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
   sizePolicy.setHorizontalStretch(0);
   sizePolicy.setVerticalStretch(0);
   sizePolicy.setHeightForWidth(fileListWidget->sizePolicy().hasHeightForWidth());
   fileListWidget->setSizePolicy(sizePolicy);

   const auto layout = new QVBoxLayout(this);
   layout->setContentsMargins(QMargins());
   layout->setSpacing(10);
   layout->addWidget(fileListWidget);

   connect(fileListWidget, &FileListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
      emit signalOpenFileCommit(mFirstShaStr, mSecondShaStr, item->text(), false);
   });
   connect(fileListWidget, &FileListWidget::signalShowFileHistory, this, &CommitDiffWidget::signalShowFileHistory);
   connect(fileListWidget, &FileListWidget::signalEditFile, this, &CommitDiffWidget::signalEditFile);
}

void CommitDiffWidget::configure(const QString &firstSha, const QString &secondSha)
{
   mFirstShaStr = firstSha;
   mSecondShaStr = secondSha;

   fileListWidget->insertFiles(mFirstShaStr, mSecondShaStr);
}
