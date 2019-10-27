#include "CreateRepoDlg.h"
#include "ui_CreateRepoDlg.h"

#include <git.h>

#include <QFileDialog>

CreateRepoDlg::CreateRepoDlg(CreateRepoDlgType type, QSharedPointer<Git> git, QWidget *parent)
   : QDialog(parent)
   , ui(new Ui::CreateRepoDlg)
   , mType(type)
   , mGit(git)
{
   QFile styles(":/stylesheet");

   if (styles.open(QIODevice::ReadOnly))
   {
      setStyleSheet(QString::fromUtf8(styles.readAll()));
      styles.close();
   }

   ui->setupUi(this);

   const auto operation = mType == CreateRepoDlgType::INIT ? QString("init") : QString("clone");
   const auto checkText = ui->chbOpen->text().arg(operation);
   ui->chbOpen->setText(checkText);

   connect(ui->leURL, &QLineEdit::returnPressed, this, &CreateRepoDlg::accept);
   connect(ui->pbBrowse, &QPushButton::clicked, this, &CreateRepoDlg::selectFolder);
   connect(ui->lePath, &QLineEdit::returnPressed, this, &CreateRepoDlg::accept);
   connect(ui->leRepoName, &QLineEdit::returnPressed, this, &CreateRepoDlg::accept);

   connect(ui->pbAccept, &QPushButton::clicked, this, &CreateRepoDlg::accept);
   connect(ui->pbCancel, &QPushButton::clicked, this, &QDialog::reject);
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
         if (ui->chbOpen->isChecked())
            emit signalRepoCloned(fullPath);

         QDialog::accept();
      }
   }
}
