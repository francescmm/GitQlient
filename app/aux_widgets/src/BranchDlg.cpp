#include "BranchDlg.h"
#include "ui_BranchDlg.h"

#include <GitBase.h>
#include <GitBranches.h>
#include <GitCache.h>
#include <GitConfig.h>
#include <GitQlientSettings.h>
#include <GitQlientStyles.h>
#include <GitRemote.h>
#include <GitStashes.h>

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

   QScopedPointer<GitRemote> git(new GitRemote(mConfig.mGit));
   const auto ret = git->getRemotes();

   if (ret.success)
   {
      const auto flag = Qt::SkipEmptyParts;
      const auto remotes = ret.output.split("\n", flag);

      if (remotes.size() > 1)
      {
         for (const auto &remote : remotes)
            ui->cbRemotes->addItem(remote);
      }
      else
         ui->cbRemotes->setVisible(false);
   }
   else
      ui->cbRemotes->setVisible(false);

   switch (mConfig.mDialogMode)
   {
      case BranchDlgMode::CREATE:
         setWindowTitle(tr("Create branch"));
         break;
      case BranchDlgMode::RENAME:
         ui->pbAccept->setText(tr("Rename"));
         setWindowTitle("Rename branch");
         break;
      case BranchDlgMode::CREATE_CHECKOUT:
         setWindowTitle(tr("Create and checkout branch"));
         ui->currentBranchFrame->setHidden(true);
         break;
      case BranchDlgMode::CREATE_FROM_COMMIT:
         setWindowTitle(tr("Create branch at commit"));
         ui->currentBranchFrame->setHidden(true);
         break;
      case BranchDlgMode::CREATE_CHECKOUT_FROM_COMMIT:
         setWindowTitle(tr("Create and checkout branch"));
         ui->currentBranchFrame->setHidden(true);
         break;
      case BranchDlgMode::STASH_BRANCH:
         setWindowTitle(tr("Stash branch"));
         break;
      case BranchDlgMode::PUSH_UPSTREAM:
         connect(ui->chbCopyRemote, &CheckBox::stateChanged, this, &BranchDlg::copyBranchName);
         ui->chbCopyRemote->setVisible(ui->cbRemotes->isHidden());
         ui->chbCopyRemote->setChecked(ui->cbRemotes->isHidden());
         setWindowTitle(tr("Push upstream branch"));
         ui->pbAccept->setText(tr("Push"));
         break;
      default:
         break;
   }

   connect(ui->leNewName, &QLineEdit::textEdited, this, &BranchDlg::replaceWhiteSpaces);
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

void BranchDlg::replaceWhiteSpaces(const QString &newName)
{
   auto whiteSpacesFreeName = newName;
   whiteSpacesFreeName.replace(' ', GitQlientSettings().globalValue("BranchSeparator").toString());
   ui->leNewName->setText(whiteSpacesFreeName);
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
      {
         ret = git->createBranchFromAnotherBranch(ui->leOldName->text(), ui->leNewName->text());

         if (ret.success)
         {
            auto type = References::Type::LocalBranch;
            auto sha = mConfig.mCache->getShaOfReference(ui->leOldName->text(), type);

            if (sha.isEmpty())
            {
               type = References::Type::RemoteBranche;
               sha = mConfig.mCache->getShaOfReference(ui->leOldName->text(), type);
            }

            if (!sha.isEmpty())
            {
               mConfig.mCache->insertReference(sha, type, ui->leNewName->text());
               emit mConfig.mCache->signalCacheUpdated();
            }
         }
      }
      else if (mConfig.mDialogMode == BranchDlgMode::CREATE_CHECKOUT)
      {
         ret = git->checkoutNewLocalBranchFromAnotherBranch(ui->leOldName->text(), ui->leNewName->text());

         if (ret.success)
         {
            mConfig.mCache->insertReference(mConfig.mGit->getLastCommit().output.trimmed(),
                                            References::Type::LocalBranch, ui->leNewName->text());
            emit mConfig.mCache->signalCacheUpdated();
         }
      }
      else if (mConfig.mDialogMode == BranchDlgMode::RENAME)
      {
         ret = git->renameBranch(ui->leOldName->text(), ui->leNewName->text());

         if (ret.success)
         {
            const auto type = References::Type::LocalBranch;
            const auto sha = mConfig.mCache->getShaOfReference(ui->leOldName->text(), type);

            mConfig.mCache->deleteReference(sha, type, ui->leOldName->text());
            mConfig.mCache->insertReference(sha, type, ui->leNewName->text());
            emit mConfig.mCache->signalCacheUpdated();
         }
      }
      else if (mConfig.mDialogMode == BranchDlgMode::CREATE_FROM_COMMIT)
      {
         ret = git->createBranchAtCommit(ui->leOldName->text(), ui->leNewName->text());

         if (ret.success)
         {
            mConfig.mCache->insertReference(ui->leOldName->text(), References::Type::LocalBranch,
                                            ui->leNewName->text());
            emit mConfig.mCache->signalCacheUpdated();
         }
      }
      else if (mConfig.mDialogMode == BranchDlgMode::CREATE_CHECKOUT_FROM_COMMIT)
      {
         ret = git->checkoutBranchFromCommit(ui->leOldName->text(), ui->leNewName->text());

         if (ret.success)
         {
            mConfig.mCache->insertReference(ui->leOldName->text(), References::Type::LocalBranch,
                                            ui->leNewName->text());
            emit mConfig.mCache->signalCacheUpdated();
         }
      }
      else if (mConfig.mDialogMode == BranchDlgMode::STASH_BRANCH)
      {
         QScopedPointer<GitStashes> git(new GitStashes(mConfig.mGit));
         ret = git->stashBranch(ui->leOldName->text(), ui->leNewName->text());
      }
      else if (mConfig.mDialogMode == BranchDlgMode::PUSH_UPSTREAM)
      {
         const auto remote = ui->cbRemotes->isVisible() ? ui->cbRemotes->currentText() : "origin";
         ret = git->pushUpstream(ui->leNewName->text(), remote, ui->leOldName->text());

         if (ret.success)
         {
            QScopedPointer<GitConfig> git(new GitConfig(mConfig.mGit));
            const auto remote = git->getRemoteForBranch(ui->leNewName->text());

            if (remote.success)
            {
               const auto sha = mConfig.mCache->getShaOfReference(ui->leOldName->text(), References::Type::LocalBranch);
               mConfig.mCache->insertReference(sha, References::Type::RemoteBranche,
                                               QString("%1/%2").arg(remote.output, ui->leNewName->text()));
               emit mConfig.mCache->signalCacheUpdated();
            }
         }
      }

      QApplication::restoreOverrideCursor();

      if (!ret.success)
      {
         QDialog::reject();

         QMessageBox msgBox(
             QMessageBox::Critical, tr("Error on branch action!"),
             QString(tr("There were problems during the branch operation. Please, see the detailed description "
                        "for more information.")),
             QMessageBox::Ok, this);
         msgBox.setDetailedText(ret.output);
         msgBox.setStyleSheet(GitQlientStyles::getStyles());
         msgBox.exec();
      }
      else
         QDialog::accept();
   }
}

void BranchDlg::copyBranchName()
{
   const auto checked = ui->chbCopyRemote->isChecked();
   ui->leNewName->setVisible(!checked);
   ui->leNewName->setText(checked ? ui->leOldName->text() : "");
}
