#include "CommitDiffWidget.h"

#include <FileListWidget.h>

#include <QVBoxLayout>
#include <QLabel>

CommitDiffWidget::CommitDiffWidget(QSharedPointer<GitBase> git, const QSharedPointer<RevisionsCache> &cache,
                                   QWidget *parent)
   : QFrame(parent)
   , mGit(git)
   , mFirstSha(new QLabel())
   , mSecondSha(new QLabel())
   , fileListWidget(new FileListWidget(mGit, cache))
{
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
}

void CommitDiffWidget::configure(const QString &firstSha, const QString &secondSha)
{
   mFirstShaStr = firstSha;
   mSecondShaStr = secondSha;
   mFirstSha->setText(mFirstShaStr);
   mSecondSha->setText(mSecondShaStr);

   fileListWidget->insertFiles(mFirstShaStr, mSecondShaStr);
}
