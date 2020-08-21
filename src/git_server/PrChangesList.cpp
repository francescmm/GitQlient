#include "PrChangesList.h"

#include <DiffHelper.h>
#include <GitHistory.h>

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
      QVector<DiffHelper::DiffChange> changes;
      auto diff = ret.output.toString();
      QPair<QStringList, QVector<DiffInfo::ChunkInfo>> oldData;
      QPair<QStringList, QVector<DiffInfo::ChunkInfo>> newData;

      changes = DiffHelper::splitDiff(diff);
   }
}
