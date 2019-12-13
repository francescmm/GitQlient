#include "GitConfigDlg.h"
#include "ui_GitConfigDlg.h"

#include <git.h>
#include <GitQlientStyles.h>

GitConfigDlg::GitConfigDlg(QSharedPointer<Git> git, QWidget *parent)
   : QDialog(parent)
   , ui(new Ui::GitConfigDlg)
   , mGit(git)
{
   ui->setupUi(this);

   setWindowFlags(Qt::FramelessWindowHint);
   setStyleSheet(GitQlientStyles::getStyles());

   const auto globalConfig = mGit->getGlobalUserInfo();
   ui->leGlobalEmail->setText(globalConfig.mUserEmail);
   ui->leGlobalName->setText(globalConfig.mUserName);

   const auto localConfig = mGit->getLocalUserInfo();
   ui->leLocalEmail->setText(localConfig.mUserEmail);
   ui->leLocalName->setText(localConfig.mUserName);

   connect(ui->checkBox, &QCheckBox::stateChanged, this, &GitConfigDlg::copyGlobalSettings);
   connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &GitConfigDlg::accepted);
}

GitConfigDlg::~GitConfigDlg()
{
   delete ui;
}

void GitConfigDlg::accepted()
{
   mGit->setGlobalUserInfo({ ui->leGlobalEmail->text(), ui->leGlobalName->text() });
   mGit->setLocalUserInfo({ ui->leLocalEmail->text(), ui->leLocalName->text() });

   close();
}

void GitConfigDlg::copyGlobalSettings(int checkState)
{
   ui->leLocalEmail->setReadOnly(checkState == Qt::Checked);
   ui->leLocalName->setReadOnly(checkState == Qt::Checked);

   if (checkState == Qt::Checked)
   {
      ui->leLocalEmail->setText(ui->leGlobalEmail->text());
      ui->leLocalName->setText(ui->leGlobalName->text());
   }
}
