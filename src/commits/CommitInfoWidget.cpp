#include <CommitInfoWidget.h>
#include <CommitInfoPanel.h>
#include <RevisionsCache.h>
#include <CommitInfo.h>
#include <FileListWidget.h>

#include <QLabel>
#include <QVBoxLayout>
#include <QDateTime>
#include <QScrollArea>

#include <QLogger.h>

using namespace QLogger;

CommitInfoWidget::CommitInfoWidget(const QSharedPointer<RevisionsCache> &cache, const QSharedPointer<GitBase> &git,
                                   QWidget *parent)
   : QWidget(parent)
   , mCache(cache)
   , mGit(git)
   , mInfoPanel(new CommitInfoPanel())
   , fileListWidget(new FileListWidget(mGit, mCache))
   , labelModCount(new QLabel())
{
   setAttribute(Qt::WA_DeleteOnClose);

   const auto labelIcon = new QLabel();
   labelIcon->setScaledContents(false);
   labelIcon->setPixmap(QIcon(":/icons/file").pixmap(15, 15));

   fileListWidget->setObjectName("fileListWidget");

   const auto headerLayout = new QHBoxLayout();
   headerLayout->setContentsMargins(5, 0, 0, 0);
   headerLayout->setSpacing(0);
   headerLayout->addWidget(labelIcon);
   headerLayout->addSpacerItem(new QSpacerItem(10, 1, QSizePolicy::Fixed, QSizePolicy::Fixed));
   headerLayout->addWidget(new QLabel(tr("Files ")));
   headerLayout->addWidget(labelModCount);
   headerLayout->addStretch();

   const auto verticalLayout = new QVBoxLayout(this);
   verticalLayout->setSpacing(10);
   verticalLayout->setContentsMargins(0, 0, 0, 0);
   verticalLayout->addWidget(mInfoPanel);
   verticalLayout->addLayout(headerLayout);
   verticalLayout->addWidget(fileListWidget);

   connect(fileListWidget, &FileListWidget::itemDoubleClicked, this,
           [this](QListWidgetItem *item) { emit signalOpenFileCommit(mCurrentSha, mParentSha, item->text()); });
   connect(fileListWidget, &FileListWidget::signalShowFileHistory, this, &CommitInfoWidget::signalShowFileHistory);
   connect(fileListWidget, &FileListWidget::signalEditFile, this, &CommitInfoWidget::signalEditFile);
}

void CommitInfoWidget::configure(const QString &sha)
{
   if (sha == mCurrentSha)
      return;

   clear();

   mCurrentSha = sha;
   mParentSha = sha;

   if (sha != CommitInfo::ZERO_SHA && !sha.isEmpty())
   {
      const auto commit = mCache->getCommitInfo(sha);

      if (!commit.sha().isEmpty())
      {
         QLog_Info("UI", QString("Loading information of the commit {%1}").arg(sha));
         mCurrentSha = commit.sha();
         mParentSha = commit.parent(0);

         mInfoPanel->configure(commit);

         fileListWidget->insertFiles(mCurrentSha, mParentSha);
         labelModCount->setText(QString("(%1)").arg(fileListWidget->count()));
      }
   }
}

QString CommitInfoWidget::getCurrentCommitSha() const
{
   return mCurrentSha;
}

void CommitInfoWidget::clear()
{
   mCurrentSha = QString();
   mParentSha = QString();

   fileListWidget->clear();
}
