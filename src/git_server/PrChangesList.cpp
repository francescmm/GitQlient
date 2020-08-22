#include "PrChangesList.h"

#include <DiffHelper.h>
#include <GitHistory.h>
#include <FileDiffView.h>

#include <QGridLayout>
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
      auto row = 0;

      if (!changes.isEmpty())
      {
         delete layout();

         const auto mainLayout = new QGridLayout();
         mainLayout->setContentsMargins(QMargins());
         mainLayout->setSpacing(10);

         for (auto &change : changes)
         {
            DiffHelper::processDiff(change.content, true, change.newData, change.oldData);

            const auto oldFile = new FileDiffView();
            oldFile->show();
            oldFile->setMinimumHeight(oldFile->getHeight());
            oldFile->setStartingLine(change.oldFileStartLine);
            oldFile->loadDiff(change.oldData.first.join("\n"), change.oldData.second);

            const auto newFile = new FileDiffView();
            newFile->show();
            newFile->setMinimumHeight(oldFile->getHeight());
            newFile->setStartingLine(change.newFileStartLine);
            newFile->loadDiff(change.newData.first.join("\n"), change.newData.second);

            mainLayout->addWidget(oldFile, row, 0);
            mainLayout->addWidget(newFile, row, 1);
            mainLayout->addItem(new QSpacerItem(1, 10, QSizePolicy::Fixed, QSizePolicy::Fixed), ++row, 0, 1, 2);

            ++row;
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
