#pragma once

#include <QFrame>

class GitBase;
class FileDiffView;

class PrChangesList : public QFrame
{
   Q_OBJECT

public:
   explicit PrChangesList(const QSharedPointer<GitBase> &git, QWidget *parent = nullptr);

   void loadData(const QString &baseBranch, const QString &headBranch);

private:
   QSharedPointer<GitBase> mGit;
   FileDiffView *mOldFile = nullptr;
   FileDiffView *mNewFile = nullptr;
};
