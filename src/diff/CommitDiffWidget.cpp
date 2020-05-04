#include "CommitDiffWidget.h"

#include <FileListWidget.h>
#include <RevisionsCache.h>

#include <QVBoxLayout>
#include <QLabel>

CommitDiffWidget::CommitDiffWidget(QSharedPointer<GitBase> git, QSharedPointer<RevisionsCache> cache, QWidget *parent)
   : QFrame(parent)
   , mGit(git)
   , mCache(cache)
   , mFirstSha(new QLabel())
   , mSecondSha(new QLabel())
   , fileListWidget(new FileListWidget(mGit, cache))
{
   setAttribute(Qt::WA_DeleteOnClose);

   mFirstSha->setObjectName("labelSha");
   mFirstSha->setAlignment(Qt::AlignCenter);

   mSecondSha->setObjectName("labelSha");
   mSecondSha->setAlignment(Qt::AlignCenter);

   QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
   sizePolicy.setHorizontalStretch(0);
   sizePolicy.setVerticalStretch(0);
   sizePolicy.setHeightForWidth(fileListWidget->sizePolicy().hasHeightForWidth());
   fileListWidget->setSizePolicy(sizePolicy);

   const auto layout = new QVBoxLayout(this);
   layout->setContentsMargins(QMargins());
   layout->setSpacing(10);
   layout->addWidget(new QLabel(tr("First SHA:")));
   layout->addWidget(mFirstSha);
   layout->addWidget(new QLabel(tr("Second SHA:")));
   layout->addWidget(mSecondSha);
   layout->addWidget(fileListWidget);

   connect(fileListWidget, &FileListWidget::itemDoubleClicked, this,
           [this](QListWidgetItem *item) { emit signalOpenFileCommit(mFirstShaStr, mSecondShaStr, item->text()); });
   connect(fileListWidget, &FileListWidget::signalShowFileHistory, this, &CommitDiffWidget::signalShowFileHistory);
   connect(fileListWidget, &FileListWidget::signalEditFile, this, &CommitDiffWidget::signalEditFile);
}

void CommitDiffWidget::configure(const QString &firstSha, const QString &secondSha)
{
   mFirstShaStr = firstSha;
   mFirstSha->setText(mFirstShaStr);

   if (mFirstShaStr != CommitInfo::ZERO_SHA)
   {
      const auto c = mCache->getCommitInfo(mFirstShaStr);
      const auto dateStr = QDateTime::fromSecsSinceEpoch(c.authorDate().toUInt()).toString("dd MMM yyyy hh:mm");
      mFirstSha->setToolTip(QString("%1 - %2\n\n%3").arg(c.author(), dateStr, c.shortLog()));
   }

   mSecondShaStr = secondSha;
   mSecondSha->setText(mSecondShaStr);

   if (mFirstShaStr != CommitInfo::ZERO_SHA)
   {
      const auto c = mCache->getCommitInfo(mSecondShaStr);
      const auto dateStr = QDateTime::fromSecsSinceEpoch(c.authorDate().toUInt()).toString("dd MMM yyyy hh:mm");
      mSecondSha->setToolTip(QString("%1 - %2\n\n%3").arg(c.author(), dateStr, c.shortLog()));
   }

   fileListWidget->insertFiles(mFirstShaStr, mSecondShaStr);
}
