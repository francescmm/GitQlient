#include "StashesContextMenu.h"

#include <BranchDlg.h>
#include <git.h>

#include <QMessageBox>

StashesContextMenu::StashesContextMenu(QSharedPointer<Git> git, const QString &stashId, QWidget *parent)
   : QMenu(parent)
   , mGit(git)
   , mStashId(stashId)
{
   connect(addAction("Branch"), &QAction::triggered, this, &StashesContextMenu::branch);
   connect(addAction("Drop"), &QAction::triggered, this, &StashesContextMenu::drop);
   connect(addAction("Clear all"), &QAction::triggered, this, &StashesContextMenu::clear);
}

void StashesContextMenu::branch()
{
   BranchDlg dlg({ mStashId, BranchDlgMode::STASH_BRANCH, mGit });
   const auto ret = dlg.exec();

   if (ret == QDialog::Accepted)
      emit signalUpdateView();
}

void StashesContextMenu::drop()
{
   const auto ret = mGit->stashDrop(mStashId);

   if (ret.success)
      emit signalUpdateView();
   else
      QMessageBox::warning(this, tr("Error on stash drop"), ret.output.toString());
}

void StashesContextMenu::clear()
{
   const auto ret = mGit->stashClear();

   if (ret.success)
      emit signalUpdateView();
   else
      QMessageBox::warning(this, tr("Error on stash branch"), ret.output.toString());
}
