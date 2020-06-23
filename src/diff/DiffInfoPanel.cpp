#include "DiffInfoPanel.h"

#include <RevisionsCache.h>

#include <QLabel>
#include <QDateTime>
#include <QVBoxLayout>

DiffInfoPanel::DiffInfoPanel(QSharedPointer<RevisionsCache> cache, QWidget *parent)
   : QFrame(parent)
   , mCache(cache)
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

   const auto infoLayout = new QHBoxLayout(this);
   infoLayout->setSpacing(0);
   infoLayout->setContentsMargins(QMargins());
   infoLayout->addStretch(1);
   infoLayout->addLayout(verticalCurrentLayout);
   infoLayout->addStretch(2);
   infoLayout->addLayout(verticalPreviousLayout);
   infoLayout->addStretch(1);
}

void DiffInfoPanel::configure(const QString &currentSha, const QString &previousSha)
{
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
}
