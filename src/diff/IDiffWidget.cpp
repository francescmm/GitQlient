#include "IDiffWidget.h"

IDiffWidget::IDiffWidget(const QSharedPointer<GitBase> &git, QSharedPointer<RevisionsCache> cache, QWidget *parent)
   : QFrame(parent)
   , mGit(git)
   , mCache(cache)
{
}
