#include "FileDiffWidget.h"

#include <GitHistory.h>
#include <FileDiffView.h>
#include <CommitInfo.h>
#include <RevisionsCache.h>
#include <DiffInfoPanel.h>

#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QScrollBar>
#include <QDateTime>

FileDiffWidget::FileDiffWidget(const QSharedPointer<GitBase> &git, QSharedPointer<RevisionsCache> cache,
                               QWidget *parent)
   : QFrame(parent)
   , mGit(git)
   , mCache(cache)
   , mNewFile(new FileDiffView())
   , mOldFile(new FileDiffView())
   , mGoPrevious(new QPushButton())
   , mGoNext(new QPushButton())
   , mDiffInfoPanel(new DiffInfoPanel(cache))

{
   setAttribute(Qt::WA_DeleteOnClose);

   mGoPrevious->setIcon(QIcon(":/icons/go_up"));
   mGoNext->setIcon(QIcon(":/icons/go_down"));

   const auto diffLayout = new QHBoxLayout();
   diffLayout->addWidget(mNewFile);
   diffLayout->addWidget(mOldFile);

   const auto vLayout = new QVBoxLayout(this);
   vLayout->setContentsMargins(QMargins());
   vLayout->setSpacing(10);
   vLayout->addWidget(mDiffInfoPanel);
   vLayout->addLayout(diffLayout);

   connect(mNewFile, &FileDiffView::signalScrollChanged, mOldFile, &FileDiffView::moveScrollBarToPos);
   connect(mOldFile, &FileDiffView::signalScrollChanged, mNewFile, &FileDiffView::moveScrollBarToPos);
}

void FileDiffWidget::clear()
{
   mNewFile->clear();
}

bool FileDiffWidget::reload()
{
   if (mCurrentSha == CommitInfo::ZERO_SHA)
      return configure(mCurrentSha, mPreviousSha, mCurrentFile);

   return false;
}

bool FileDiffWidget::configure(const QString &currentSha, const QString &previousSha, const QString &file)
{
   mCurrentFile = file;
   mCurrentSha = currentSha;
   mPreviousSha = previousSha;

   mDiffInfoPanel->configure(currentSha, previousSha);

   auto destFile = file;

   if (destFile.contains("-->"))
      destFile = destFile.split("--> ").last().split("(").first().trimmed();

   QScopedPointer<GitHistory> git(new GitHistory(mGit));
   auto text = git->getFileDiff(currentSha == CommitInfo::ZERO_SHA ? QString() : currentSha, previousSha, destFile);

   return mNewFile->loadDiff(text) && mOldFile->loadDiff(text);
}
