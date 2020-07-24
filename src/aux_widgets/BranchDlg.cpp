#include "BranchDlg.h"
#include "ui_BranchDlg.h"

#include <GitBranches.h>
#include <GitStashes.h>
#include <GitQlientStyles.h>

#include <QFile>
#include <QMessageBox>

#include <utility>

BranchDlg::BranchDlg(BranchDlgConfig config, QWidget *parent)
   : QDialog(parent)
   , ui(new Ui::BranchDlg)
   , mConfig(std::move(config))
{
   setStyleSheet(GitQlientStyles::getStyles());

   ui->setupUi(this);
   ui->leOldName->setText(mConfig.mCurrentBranchName);

   ui->chbCopyRemote->setHidden(true);

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
         ui->chbCopyRemote->setVisible(true);
         connect(ui->chbCopyRemote, &CheckBox::clicked, this, &BranchDlg::copyBranchName);
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
      GitExecResult ret;

      if (mConfig.mDialogMode == BranchDlgMode::CREATE)
         ret = git->createBranchFromAnotherBranch(ui->leOldName->text(), ui->leNewName->text());
      else if (mConfig.mDialogMode == BranchDlgMode::CREATE_CHECKOUT)
         ret = git->checkoutNewLocalBranch(ui->leNewName->text());
      else if (mConfig.mDialogMode == BranchDlgMode::RENAME)
         ret = git->renameBranch(ui->leOldName->text(), ui->leNewName->text());
      else if (mConfig.mDialogMode == BranchDlgMode::CREATE_FROM_COMMIT)
         ret = git->createBranchAtCommit(ui->leOldName->text(), ui->leNewName->text());
      else if (mConfig.mDialogMode == BranchDlgMode::STASH_BRANCH)
      {
         QScopedPointer<GitStashes> git(new GitStashes(mConfig.mGit));
         ret = git->stashBranch(ui->leOldName->text(), ui->leNewName->text());
      }
      else if (mConfig.mDialogMode == BranchDlgMode::PUSH_UPSTREAM)
         ret = git->pushUpstream(ui->leNewName->text());

      QApplication::restoreOverrideCursor();

      QDialog::accept();

      if (!ret.success)
      {
         QMessageBox msgBox(
             QMessageBox::Critical, tr("Error on branch action!"),
             QString("There were problems during the branch operation. Please, see the detailed description "
                     "for more information."),
             QMessageBox::Ok, this);
         msgBox.setDetailedText(ret.output.toString());
         msgBox.setStyleSheet(GitQlientStyles::getStyles());
         msgBox.exec();
      }
   }
}

void BranchDlg::copyBranchName()
{
   const auto remote = ui->leOldName->text();
   ui->leNewName->setText(remote);
}
