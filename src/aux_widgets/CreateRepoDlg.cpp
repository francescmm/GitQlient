#include "CreateRepoDlg.h"
#include "ui_CreateRepoDlg.h"

#include <GitBase.h>
#include <GitConfig.h>
#include <GitQlientStyles.h>
#include <GitQlientSettings.h>

#include <QFileDialog>
#include <QMessageBox>
#include <QLogger.h>

using namespace QLogger;

CreateRepoDlg::CreateRepoDlg(CreateRepoDlgType type, QSharedPointer<GitConfig> git, QWidget *parent)
   : QDialog(parent)
   , ui(new Ui::CreateRepoDlg)
   , mType(type)
   , mGit(git)
{
   setStyleSheet(GitQlientStyles::getStyles());

   ui->setupUi(this);

   if (mType == CreateRepoDlgType::INIT)
      ui->leURL->setHidden(true);

   const auto operation = mType == CreateRepoDlgType::INIT ? QString("init") : QString("clone");
   const auto checkText = ui->chbOpen->text().arg(operation);
   ui->chbOpen->setText(checkText);

   setWindowTitle(
       QString("%1 repository").arg(mType == CreateRepoDlgType::INIT ? QString("Initialize") : QString("Clone")));

   connect(ui->leURL, &QLineEdit::returnPressed, this, &CreateRepoDlg::accept);
   connect(ui->leURL, &QLineEdit::textChanged, this, &CreateRepoDlg::addDefaultName);
   connect(ui->pbBrowse, &QPushButton::clicked, this, &CreateRepoDlg::selectFolder);
   connect(ui->lePath, &QLineEdit::returnPressed, this, &CreateRepoDlg::accept);
   connect(ui->leRepoName, &QLineEdit::returnPressed, this, &CreateRepoDlg::accept);
   connect(ui->pbAccept, &QPushButton::clicked, this, &CreateRepoDlg::accept);
   connect(ui->pbCancel, &QPushButton::clicked, this, &QDialog::reject);
   connect(ui->cbGitUser, &CheckBox::clicked, this, &CreateRepoDlg::showGitControls);

   showGitControls();
}

CreateRepoDlg::~CreateRepoDlg()
{
   delete ui;
}

void CreateRepoDlg::selectFolder()
{
   const QString dirName(QFileDialog::getExistingDirectory(this, "Choose the directory of a Git project"));

   if (!dirName.isEmpty())
   {
      QDir d(dirName);

      ui->lePath->setText(d.absolutePath());
   }
}

void CreateRepoDlg::addDefaultName(const QString &url)
{
   static const QString extension(".git");
   if (url.endsWith(extension))
   {
      const auto lastDashIndex = url.lastIndexOf("/");
      const auto projectName = url.mid(lastDashIndex + 1, url.size() - lastDashIndex - extension.size() - 1);

      ui->leRepoName->setText(projectName);
   }
}

void CreateRepoDlg::showGitControls()
{
   const auto checkedState = ui->cbGitUser->isChecked();

   ui->leGitName->setVisible(checkedState);
   ui->leGitEmail->setVisible(checkedState);
}

void CreateRepoDlg::accept()
{
   auto path = ui->lePath->text();
   auto repoName = ui->leRepoName->text();

   if (!path.isEmpty() && !repoName.isEmpty())
   {
      repoName.replace(" ", "\\ ");
      const auto fullPath = path.append("/").append(repoName);

      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

      GitExecResult ret;
      QString actionApplied;

      if (mType == CreateRepoDlgType::CLONE)
      {
         const auto url = ui->leURL->text();

         if (!url.isEmpty())
         {
            actionApplied = "clone";

            QDir dir(fullPath);

            if (!dir.exists())
               dir.mkpath(fullPath);

            ret = mGit->clone(url, fullPath);
         }
         else
         {
            const auto msg = QString("You need to provider a URL to clone a repository.");

            QMessageBox::critical(this, tr("Nor URL provided"), msg);

            QLog_Error("UI", msg);
         }
      }
      else if (mType == CreateRepoDlgType::INIT)
      {
         actionApplied = "init";
         ret = mGit->initRepo(fullPath);
      }

      QApplication::restoreOverrideCursor();

      if (ret.success)
      {
         if (ui->cbGitUser->isChecked())
            mGit->setLocalUserInfo({ ui->leGitName->text(), ui->leGitEmail->text() });

         if (ui->chbOpen->isChecked())
            emit signalOpenWhenFinish(fullPath);

         QDialog::accept();
      }
      else
      {
         const auto msg = ret.output.toString();

         QMessageBox::critical(this, tr("Error when %1").arg(actionApplied), msg);

         QLog_Error("UI", msg);
      }
   }
}
