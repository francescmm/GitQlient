#include "FileDiffWidget.h"

#include <GitHistory.h>
#include <FileDiffView.h>
#include <FileDiffHighlighter.h>
#include <CommitInfo.h>
#include <RevisionsCache.h>

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
   , mDiffView(new FileDiffView())
   , mGoPrevious(new QPushButton())
   , mGoNext(new QPushButton())
   , mLabelCurrentSha(new QLabel())
   , mLabelCurrentTitle(new QLabel())
   , mLabelCurrentAuthor(new QLabel())
   , mLabelCurrentDateTime(new QLabel())
   , mLabelCurrentEmail(new QLabel())
   , mLabelPreviousSha(new QLabel())
   , mLabelPreviousTitle(new QLabel())
   , mLabelPreviousAuthor(new QLabel())
   , mLabelPreviousDateTime(new QLabel())
   , mLabelPreviousEmail(new QLabel())

{
   setAttribute(Qt::WA_DeleteOnClose);

   mDiffHighlighter = new FileDiffHighlighter(mDiffView->document());

   mGoPrevious->setIcon(QIcon(":/icons/go_up"));
   mGoNext->setIcon(QIcon(":/icons/go_down"));

   mLabelCurrentSha->setObjectName("labelSha");
   mLabelCurrentSha->setAlignment(Qt::AlignCenter);
   mLabelCurrentSha->setWordWrap(true);

   QFont font1;
   font1.setBold(true);
   font1.setWeight(75);
   mLabelCurrentTitle->setFont(font1);
   mLabelCurrentTitle->setAlignment(Qt::AlignCenter);
   mLabelCurrentTitle->setWordWrap(true);
   mLabelCurrentTitle->setObjectName("labelTitle");

   const auto commitInfoFrame = new QFrame();
   commitInfoFrame->setObjectName("commitInfoFrame");

   const auto layout = new QVBoxLayout(commitInfoFrame);
   layout->setSpacing(15);
   layout->setContentsMargins(0, 0, 0, 0);
   layout->addWidget(mLabelCurrentAuthor);
   layout->addWidget(mLabelCurrentDateTime);
   layout->addWidget(mLabelCurrentEmail);

   const auto verticalCurrentLayout = new QVBoxLayout();
   verticalCurrentLayout->setSpacing(10);
   verticalCurrentLayout->setContentsMargins(0, 0, 0, 0);
   verticalCurrentLayout->addWidget(mLabelCurrentSha);
   verticalCurrentLayout->addWidget(mLabelCurrentTitle);
   verticalCurrentLayout->addWidget(commitInfoFrame);

   mLabelPreviousSha->setObjectName("labelSha");
   mLabelPreviousSha->setAlignment(Qt::AlignCenter);
   mLabelPreviousSha->setWordWrap(true);

   font1.setBold(true);
   font1.setWeight(75);
   mLabelPreviousTitle->setFont(font1);
   mLabelPreviousTitle->setAlignment(Qt::AlignCenter);
   mLabelPreviousTitle->setWordWrap(true);
   mLabelPreviousTitle->setObjectName("labelTitle");

   const auto previousCommitInfoFrame = new QFrame();
   previousCommitInfoFrame->setObjectName("commitInfoFrame");

   const auto anotherLayout = new QVBoxLayout(previousCommitInfoFrame);
   anotherLayout->setSpacing(15);
   anotherLayout->setContentsMargins(0, 0, 0, 0);
   anotherLayout->addWidget(mLabelPreviousAuthor);
   anotherLayout->addWidget(mLabelPreviousDateTime);
   anotherLayout->addWidget(mLabelPreviousEmail);

   const auto verticalPreviousLayout = new QVBoxLayout();
   verticalPreviousLayout->setSpacing(10);
   verticalPreviousLayout->setContentsMargins(0, 0, 0, 0);
   verticalPreviousLayout->addWidget(mLabelPreviousSha);
   verticalPreviousLayout->addWidget(mLabelPreviousTitle);
   verticalPreviousLayout->addWidget(previousCommitInfoFrame);

   const auto infoLayout = new QHBoxLayout();
   infoLayout->setSpacing(0);
   infoLayout->setContentsMargins(QMargins());
   infoLayout->addLayout(verticalCurrentLayout);
   infoLayout->addStretch();
   infoLayout->addLayout(verticalPreviousLayout);

   const auto vLayout = new QVBoxLayout(this);
   vLayout->setContentsMargins(QMargins());
   vLayout->setSpacing(10);
   vLayout->addLayout(infoLayout);
   vLayout->addWidget(mDiffView);
}

void FileDiffWidget::clear()
{
   mDiffView->clear();
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

   const auto currentCommit = mCache->getCommitInfo(currentSha);
   mLabelCurrentSha->setText(currentCommit.sha());
   mLabelCurrentTitle->setText(currentCommit.shortLog());
   mLabelCurrentAuthor->setText(currentCommit.author());
   mLabelCurrentDateTime->setText(
       QDateTime::fromSecsSinceEpoch(currentCommit.authorDate().toInt()).toString("dd MMM yyyy hh:mm"));
   mLabelCurrentEmail->setText("");

   const auto previousCommit = mCache->getCommitInfo(previousSha);
   mLabelPreviousSha->setText(previousCommit.sha());
   mLabelPreviousTitle->setText(previousCommit.shortLog());
   mLabelPreviousAuthor->setText(previousCommit.author());
   mLabelPreviousDateTime->setText(
       QDateTime::fromSecsSinceEpoch(previousCommit.authorDate().toInt()).toString("dd MMM yyyy hh:mm"));
   mLabelPreviousEmail->setText("");

   auto destFile = file;

   if (destFile.contains("-->"))
      destFile = destFile.split("--> ").last().split("(").first().trimmed();

   QScopedPointer<GitHistory> git(new GitHistory(mGit));
   auto text = git->getFileDiff(currentSha == CommitInfo::ZERO_SHA ? QString() : currentSha, previousSha, destFile);
   auto lines = text.split("\n");

   for (auto i = 0; !lines.isEmpty() && i < 5; ++i)
      lines.takeFirst();

   if (!lines.isEmpty())
   {
      text = lines.join("\n");

      const auto pos = mDiffView->verticalScrollBar()->value();
      mDiffView->setPlainText(text);

      mRowIndex = 0;

      mDiffView->moveCursor(QTextCursor::Start);

      mDiffView->verticalScrollBar()->setValue(pos);

      return true;
   }

   return false;
}
