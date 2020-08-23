#include "PrChangesList.h"

#include <DiffHelper.h>
#include <GitHistory.h>
#include <FileDiffView.h>
#include <PrChangeListItem.h>

#include <QGridLayout>
#include <QLabel>
#include <QScrollArea>

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
         mainLayout->setContentsMargins(10, 10, 10, 10);
         mainLayout->setSpacing(0);

         for (auto &change : changes)
         {
            mainLayout->addWidget(new PrChangeListItem(change));
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
