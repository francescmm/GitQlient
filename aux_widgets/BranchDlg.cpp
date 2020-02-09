#include "BranchDlg.h"
#include "ui_BranchDlg.h"

#include <GitBranches.h>
#include <GitStashes.h>
#include <GitQlientStyles.h>

#include <QFile>
#include <utility>

BranchDlg::BranchDlg(BranchDlgConfig config, QWidget *parent)
   : QDialog(parent)
   , ui(new Ui::BranchDlg)
   , mConfig(std::move(config))
{
   setStyleSheet(GitQlientStyles::getStyles());

   ui->setupUi(this);
   ui->leOldName->setText(mConfig.mCurrentBranchName);

   switch (mConfig.mDialogMode)
   {
      case BranchDlgMode::CREATE:
         setWindowTitle("Create branch");
         break;
      case BranchDlgMode::RENAME:
         ui->pbAccept->setText(tr("Rename"));
         setWindowTitle("Rename branch");
         break;
      case BranchDlgMode::CREATE_CHECKOUT:
         setWindowTitle("Create and checkout branch");
         ui->leOldName->setHidden(true);
         break;
      case BranchDlgMode::CREATE_FROM_COMMIT:
         setWindowTitle("Create branch at commit");
         ui->leOldName->setHidden(true);
         break;
      case BranchDlgMode::STASH_BRANCH:
         setWindowTitle("Stash branch");
         break;
      case BranchDlgMode::PUSH_UPSTREAM:
         setWindowTitle("Push upstream branch");
         ui->pbAccept->setText(tr("Push"));
         break;
      default:
         break;
   }

   connect(ui->leNewName, &QLineEdit::editingFinished, this, &BranchDlg::checkNewBranchName);
   connect(ui->leNewName, &QLineEdit::returnPressed, this, &BranchDlg::accept);
   connect(ui->pbAccept, &QPushButton::clicked, this, &BranchDlg::accept);
   connect(ui->pbCancel, &QPushButton::clicked, this, &BranchDlg::reject);
}

BranchDlg::~BranchDlg()
{
   delete ui;
}

void BranchDlg::checkNewBranchName()
{
   if (ui->leNewName->text() == ui->leOldName->text() && mConfig.mDialogMode != BranchDlgMode::PUSH_UPSTREAM)
      ui->leNewName->setStyleSheet("border: 1px solid red;");
}

void BranchDlg::accept()
{
   if (ui->leNewName->text() == ui->leOldName->text() && mConfig.mDialogMode != BranchDlgMode::PUSH_UPSTREAM)
      ui->leNewName->setStyleSheet("border: 1px solid red;");
   else
   {
      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

      QScopedPointer<GitBranches> git(new GitBranches(mConfig.mGit));

      if (mConfig.mDialogMode == BranchDlgMode::CREATE)
         git->createBranchFromAnotherBranch(ui->leOldName->text(), ui->leNewName->text());
      else if (mConfig.mDialogMode == BranchDlgMode::CREATE_CHECKOUT)
         git->checkoutNewLocalBranch(ui->leNewName->text());
      else if (mConfig.mDialogMode == BranchDlgMode::RENAME)
         git->renameBranch(ui->leOldName->text(), ui->leNewName->text());
      else if (mConfig.mDialogMode == BranchDlgMode::CREATE_FROM_COMMIT)
         git->createBranchAtCommit(ui->leOldName->text(), ui->leNewName->text());
      else if (mConfig.mDialogMode == BranchDlgMode::STASH_BRANCH)
      {
         QScopedPointer<GitStashes> git(new GitStashes(mConfig.mGit));
         git->stashBranch(ui->leOldName->text(), ui->leNewName->text());
      }
      else if (mConfig.mDialogMode == BranchDlgMode::PUSH_UPSTREAM)
         git->pushUpstream(ui->leNewName->text());

      QApplication::restoreOverrideCursor();

      QDialog::accept();
   }
}
