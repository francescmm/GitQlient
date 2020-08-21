#include "PrChangesList.h"

PrChangesList::PrChangesList(const QSharedPointer<GitBase> &git, QWidget *parent)
   : QFrame(parent)
   , mGit(git)
{
}

void PrChangesList::loadData(const QString &headBranch, const QString &baseBranch)
{
   Q_UNUSED(headBranch);
   Q_UNUSED(baseBranch);
}
