#include "BranchDlg.h"
#include "ui_BranchDlg.h"

#include "git.h"

#include <QFile>

BranchDlg::BranchDlg(const QString &currentName, BranchDlgMode mode, QWidget *parent)
   : QDialog(parent)
   , ui(new Ui::BranchDlg)
   , mMode(mode)
{
   QFile styles(":/stylesheet.css");

   if (styles.open(QIODevice::ReadOnly))
   {
      setStyleSheet(QString::fromUtf8(styles.readAll()));
      styles.close();
   }

   ui->setupUi(this);
   ui->leOldName->setText(currentName);

   if (mMode == BranchDlgMode::CREATE)
      setWindowTitle("Create branch");
   else if (mMode == BranchDlgMode::RENAME)
   {
      ui->pbAccept->setText(tr("Rename"));
      setWindowTitle("Rename branch");
   }
   else if (mMode == BranchDlgMode::CREATE_CHECKOUT)
   {
      setWindowTitle("Create and checkout branch");
      ui->leOldName->setHidden(true);
   }
   else if (mMode == BranchDlgMode::CREATE_FROM_COMMIT)
   {
      setWindowTitle("Create branch at commit");
      ui->leOldName->setHidden(true);
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

      if (mMode == BranchDlgMode::CREATE)
         Git::getInstance()->createBranchFromAnotherBranch(ui->leOldName->text(), ui->leNewName->text());
      else if (mMode == BranchDlgMode::CREATE_CHECKOUT)
         Git::getInstance()->checkoutNewLocalBranch(ui->leNewName->text());
      else if (mMode == BranchDlgMode::RENAME)
         Git::getInstance()->renameBranch(ui->leOldName->text(), ui->leNewName->text());
      else if (mMode == BranchDlgMode::CREATE_FROM_COMMIT)
         Git::getInstance()->createBranchAtCommit(ui->leOldName->text(), ui->leNewName->text());

      QApplication::restoreOverrideCursor();

      QDialog::accept();
   }
}
