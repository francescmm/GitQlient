#include <CommitInfoWidget.h>
#include <CommitInfoPanel.h>
#include <GitCache.h>
#include <CommitInfo.h>
#include <FileListWidget.h>

#include <QLabel>
#include <QVBoxLayout>
#include <QDateTime>

#include <QLogger.h>

using namespace QLogger;

CommitInfoWidget::CommitInfoWidget(const QSharedPointer<GitCache> &cache, const QSharedPointer<GitBase> &git,
                                   QWidget *parent)
   : QWidget(parent)
   , mCache(cache)
   , mGit(git)
   , mInfoPanel(new CommitInfoPanel())
   , fileListWidget(new FileListWidget(mGit, mCache))
{
   setAttribute(Qt::WA_DeleteOnClose);

   fileListWidget->setObjectName("fileListWidget");

   const auto verticalLayout = new QVBoxLayout(this);
   verticalLayout->setSpacing(0);
   verticalLayout->setContentsMargins(0, 0, 0, 0);
   verticalLayout->addWidget(mInfoPanel);
   verticalLayout->addWidget(fileListWidget);

   connect(fileListWidget, &FileListWidget::itemDoubleClicked, this,
           [this](QListWidgetItem *item) { emit signalOpenFileCommit(mCurrentSha, mParentSha, item->text(), false); });
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
