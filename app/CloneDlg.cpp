#include "CloneDlg.h"
#include "ui_CloneDlg.h"

#include <git.h>

#include <QFileDialog>

CloneDlg::CloneDlg(QSharedPointer<Git> git, QWidget *parent)
   : QDialog(parent)
   , ui(new Ui::CloneDlg)
   , mGit(git)
{
   ui->setupUi(this);

   connect(ui->leURL, &QLineEdit::returnPressed, this, &CloneDlg::accept);
   connect(ui->pbBrowse, &QPushButton::clicked, this, &CloneDlg::selectFolder);
   connect(ui->lePath, &QLineEdit::returnPressed, this, &CloneDlg::accept);
   connect(ui->leRepoName, &QLineEdit::returnPressed, this, &CloneDlg::accept);

   connect(ui->pbAccept, &QPushButton::clicked, this, &CloneDlg::accept);
   connect(ui->pbCancel, &QPushButton::clicked, this, &QDialog::reject);
}

CloneDlg::~CloneDlg()
{
   delete ui;
}

void CloneDlg::selectFolder()
{
   const QString dirName(QFileDialog::getExistingDirectory(this, "Choose the directory of a Git project"));

   if (!dirName.isEmpty())
   {
      QDir d(dirName);

      ui->lePath->setText(d.absolutePath());
   }
}

void CloneDlg::accept()
{
   auto url = ui->leURL->text();
   auto path = ui->lePath->text();
   auto repoName = ui->leRepoName->text();

   if (!url.isEmpty() && !path.isEmpty() && !repoName.isEmpty())
   {
      repoName.replace(" ", "\\ ");
      const auto fullPath = path.append("/").append(repoName);

      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
      auto ret = mGit->clone(url, fullPath);
      QApplication::restoreOverrideCursor();

      if (ret)
      {
         if (ui->chbOpen->isChecked())
            emit signalRepoCloned(fullPath);

         QDialog::accept();
      }
   }
}
