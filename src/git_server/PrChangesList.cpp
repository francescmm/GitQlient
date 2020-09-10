#include "PrChangesList.h"

#include <DiffHelper.h>
#include <GitHistory.h>
#include <FileDiffView.h>
#include <PrChangeListItem.h>
#include <PullRequest.h>

#include <QGridLayout>
#include <QLabel>
#include <QScrollArea>

using namespace GitServer;

PrChangesList::PrChangesList(const QSharedPointer<GitBase> &git, QWidget *parent)
   : QFrame(parent)
   , mGit(git)
{
}

void PrChangesList::loadData(const QString &baseBranch, const QString &headBranch)
{
   Q_UNUSED(headBranch);
   Q_UNUSED(baseBranch);

   QScopedPointer<GitHistory> git(new GitHistory(mGit));
   const auto ret = git->getBranchesDiff(baseBranch, headBranch);

   if (ret.success)
   {
      auto diff = ret.output.toString();
      auto changes = DiffHelper::splitDiff(diff);

      if (!changes.isEmpty())
      {
         delete layout();

         const auto mainLayout = new QVBoxLayout();
         mainLayout->setContentsMargins(20, 20, 20, 20);
         mainLayout->setSpacing(0);

         for (auto &change : changes)
         {
            const auto changeListItem = new PrChangeListItem(change);
            connect(changeListItem, &PrChangeListItem::gotoReview, this, &PrChangesList::gotoReview);

            mListItems.append(changeListItem);

            mainLayout->addWidget(changeListItem);
            mainLayout->addSpacing(10);
         }

         const auto mIssuesFrame = new QFrame();
         mIssuesFrame->setObjectName("IssuesViewFrame");
         mIssuesFrame->setLayout(mainLayout);

         const auto mScroll = new QScrollArea();
         mScroll->setWidgetResizable(true);
         mScroll->setWidget(mIssuesFrame);

         const auto aLayout = new QVBoxLayout(this);
         aLayout->setContentsMargins(QMargins());
         aLayout->setSpacing(0);
         aLayout->addWidget(mScroll);
      }
   }
}

void PrChangesList::onReviewsReceived(PullRequest pr)
{
   using Bookmark = QPair<int, int>;
   QMultiMap<QString, Bookmark> bookmarksPerFile;

   auto comments = pr.reviewComment;

   for (const auto &review : qAsConst(pr.reviews))
   {
      QMap<int, QVector<CodeReview>> reviews;
      QVector<int> codeReviewIds;

      auto iter = comments.begin();

      while (iter != comments.end())
      {
         if (iter->reviewId == review.id)
         {
            codeReviewIds.append(iter->id);
            reviews[iter->id].append(*iter);
            comments.erase(iter);
         }
         else if (codeReviewIds.contains(iter->replyToId))
         {
            reviews[iter->replyToId].append(*iter);
            comments.erase(iter);
         }
         else
            ++iter;
      }

      if (!reviews.isEmpty())
      {
         for (auto &codeReviews : reviews)
         {
            std::sort(codeReviews.begin(), codeReviews.end(),
                      [](const CodeReview &r1, const CodeReview &r2) { return r1.creation < r2.creation; });

            const auto first = codeReviews.constFirst();

            if (!first.outdated)
               bookmarksPerFile.insert(first.diff.file, { first.diff.line, first.id });
            else
               bookmarksPerFile.insert(first.diff.file, { first.diff.originalLine, first.id });
         }
      }
   }

   for (auto iter : qAsConst(mListItems))
   {
      QMap<int, int> bookmarks;
      const auto values = bookmarksPerFile.values(iter->getFileName());

      for (auto bookmark : qAsConst(values))
      {
         if (bookmark.first >= iter->getStartingLine() && bookmark.first <= iter->getEndingLine())
            bookmarks.insert(bookmark.first, bookmark.second);
      }

      if (!bookmarks.isEmpty())
         iter->setBookmarks(bookmarks);
   }
}

void PrChangesList::addLinks(PullRequest pr, const QMap<int, int> &reviewLinkToComments)
{
   QMultiMap<QString, QPair<int, int>> bookmarksPerFile;

   for (auto reviewId : reviewLinkToComments)
   {
      for (const auto &review : qAsConst(pr.reviewComment))
      {
         if (review.id == reviewId)
         {
            if (!review.outdated)
               bookmarksPerFile.insert(review.diff.file, { review.diff.line, review.id });
            else
               bookmarksPerFile.insert(review.diff.file, { review.diff.originalLine, review.id });

            break;
         }
      }
   }

   for (const auto &iter : qAsConst(mListItems))
   {
      QMap<int, int> bookmarks;

      const auto values = bookmarksPerFile.values(iter->getFileName());
      for (const auto &bookmark : values)
      {
         if (bookmark.first >= iter->getStartingLine() && bookmark.first <= iter->getEndingLine())
            bookmarks.insert(bookmark.first, reviewLinkToComments.key(bookmark.second));
      }

      if (!bookmarks.isEmpty())
         iter->setBookmarks(bookmarks);
   }
}
