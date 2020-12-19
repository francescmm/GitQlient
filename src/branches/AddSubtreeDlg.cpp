#include "AddSubtreeDlg.h"
#include "ui_AddSubmoduleDlg.h"

#include <GitSubtree.h>
#include <GitQlientStyles.h>
#include <QLogger.h>

#include <QMessageBox>

using namespace QLogger;

AddSubtreeDlg::AddSubtreeDlg(const QSharedPointer<GitBase> &git, QWidget *parent)
   : QDialog(parent)
   , ui(new Ui::AddSubmoduleDlg)
   , mGit(git)
{
   setStyleSheet(GitQlientStyles::getStyles());

   ui->setupUi(this);

   setWindowTitle("Add subtree");

   connect(ui->lePath, &QLineEdit::returnPressed, this, &AddSubtreeDlg::accept);
   connect(ui->leUrl, &QLineEdit::returnPressed, this, &AddSubtreeDlg::accept);
   connect(ui->leUrl, &QLineEdit::editingFinished, this, &AddSubtreeDlg::proposeName);
   connect(ui->pbAccept, &QPushButton::clicked, this, &AddSubtreeDlg::accept);
   connect(ui->pbCancel, &QPushButton::clicked, this, &QDialog::reject);
}

AddSubtreeDlg::~AddSubtreeDlg()
{
   delete ui;
}

void AddSubtreeDlg::accept()
{
   const auto remoteName = ui->lePath->text();
   const auto remoteUrl = ui->leUrl->text();

   QScopedPointer<GitSubtree> git(new GitSubtree(mGit));

   if (remoteName.isEmpty() || remoteUrl.isEmpty())
   {
      QMessageBox::warning(
          this, tr("Invalid fields"),
          tr("The information provided is incorrect. Please fix the URL and/or the name and submit again."));
   }
   else
   {
      const auto ret = git->add(remoteUrl, remoteName);

      if (ret.success)
         QDialog::accept();
      else
         QMessageBox::warning(this, tr("Error when adding a subtree."), ret.output.toString());
   }
}

void AddSubtreeDlg::proposeName()
{
   auto url = ui->leUrl->text();
   QString proposedName;

   if (url.startsWith("https"))
   {
      url.remove("https://");
      const auto fields = url.split("/");

      if (fields.count() > 1)
      {
         proposedName = fields.at(2);
         proposedName = proposedName.split(".").constFirst();
      }
   }
   else if (url.contains("@"))
   {
      const auto fields = url.split(":");

      if (fields.count() > 0)
      {
         proposedName = fields.constLast().split("/").constLast();
         proposedName = proposedName.split(".").constFirst();
      }
   }

   ui->lePath->setText(proposedName);
}
