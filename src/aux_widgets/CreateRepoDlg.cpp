#include "CreateRepoDlg.h"
#include "ui_CreateRepoDlg.h"

#include <GitBase.h>
#include <GitConfig.h>
#include <GitQlientStyles.h>
#include <GitQlientSettings.h>

#include <QFileDialog>

CreateRepoDlg::CreateRepoDlg(CreateRepoDlgType type, QSharedPointer<GitConfig> git, QWidget *parent)
   : QDialog(parent)
   , ui(new Ui::CreateRepoDlg)
   , mType(type)
   , mGit(git)
{
   setStyleSheet(GitQlientStyles::getStyles());

   ui->setupUi(this);

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
   connect(ui->cbGitUser, &QCheckBox::clicked, this, &CreateRepoDlg::showGitControls);

   GitQlientSettings settings;
   const auto configGitUser = settings.value("GitConfigRepo", true).toBool();
   ui->cbGitUser->setChecked(configGitUser);
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

   GitQlientSettings settings;
   settings.setValue("GitConfigRepo", checkedState);
}

void CreateRepoDlg::accept()
{
   auto url = ui->leURL->text();
   auto path = ui->lePath->text();
   auto repoName = ui->leRepoName->text();

   if (!url.isEmpty() && !path.isEmpty() && !repoName.isEmpty())
   {
      repoName.replace(" ", "\\ ");
      const auto fullPath = path.append("/").append(repoName);

      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

      auto ret = false;
      if (mType == CreateRepoDlgType::CLONE)
         ret = mGit->clone(url, fullPath);
      else if (mType == CreateRepoDlgType::INIT)
         ret = mGit->initRepo(fullPath);

      QApplication::restoreOverrideCursor();

      if (ret)
      {
         if (ui->cbGitUser->isChecked())
            mGit->setLocalUserInfo({ ui->leGitName->text(), ui->leGitEmail->text() });

         if (ui->chbOpen->isChecked())
            emit signalOpenWhenFinish(fullPath);

         QDialog::accept();
      }
   }
}
