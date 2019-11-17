#include "BranchDlg.h"
#include "ui_BranchDlg.h"

#include "git.h"

#include <QFile>

BranchDlg::BranchDlg(const BranchDlgConfig &config, QWidget *parent)
   : QDialog(parent)
   , ui(new Ui::BranchDlg)
   , mConfig(config)
{
   QFile styles(":/stylesheet");

   if (styles.open(QIODevice::ReadOnly))
   {
      QFile colors(":/stylesheet_colors");
      QString colorsCss;

      if (colors.open(QIODevice::ReadOnly))
      {
         colorsCss = colors.readAll();
         colors.close();
      }

      setStyleSheet(styles.readAll() + colorsCss);
      styles.close();
   }

   ui->setupUi(this);
   ui->leOldName->setText(mConfig.mCurrentBranchName);

   if (mConfig.mDialogMode == BranchDlgMode::CREATE)
      setWindowTitle("Create branch");
   else if (mConfig.mDialogMode == BranchDlgMode::RENAME)
   {
      ui->pbAccept->setText(tr("Rename"));
      setWindowTitle("Rename branch");
   }
   else if (mConfig.mDialogMode == BranchDlgMode::CREATE_CHECKOUT)
   {
      setWindowTitle("Create and checkout branch");
      ui->leOldName->setHidden(true);
   }
   else if (mConfig.mDialogMode == BranchDlgMode::CREATE_FROM_COMMIT)
   {
      setWindowTitle("Create branch at commit");
      ui->leOldName->setHidden(true);
   }
   else if (mConfig.mDialogMode == BranchDlgMode::STASH_BRANCH)
   {
      setWindowTitle("Stash branch");
      // ui->leOldName->setHidden(true);
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
   if (ui->leNewName->text() == ui->leOldName->text())
      ui->leNewName->setStyleSheet("border: 1px solid red;");
}

void BranchDlg::accept()
{
   if (ui->leNewName->text() == ui->leOldName->text())
      ui->leNewName->setStyleSheet("border: 1px solid red;");
   else
   {
      QByteArray output;

      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

      if (mConfig.mDialogMode == BranchDlgMode::CREATE)
         mConfig.mGit->createBranchFromAnotherBranch(ui->leOldName->text(), ui->leNewName->text());
      else if (mConfig.mDialogMode == BranchDlgMode::CREATE_CHECKOUT)
         mConfig.mGit->checkoutNewLocalBranch(ui->leNewName->text());
      else if (mConfig.mDialogMode == BranchDlgMode::RENAME)
         mConfig.mGit->renameBranch(ui->leOldName->text(), ui->leNewName->text());
      else if (mConfig.mDialogMode == BranchDlgMode::CREATE_FROM_COMMIT)
         mConfig.mGit->createBranchAtCommit(ui->leOldName->text(), ui->leNewName->text());
      else if (mConfig.mDialogMode == BranchDlgMode::STASH_BRANCH)
         mConfig.mGit->stashBranch(ui->leOldName->text(), ui->leNewName->text());

      QApplication::restoreOverrideCursor();

      QDialog::accept();
   }
}
