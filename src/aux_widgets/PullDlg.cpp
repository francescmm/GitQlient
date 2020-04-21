#include "PullDlg.h"
#include "ui_PullDlg.h"

#include <GitQlientStyles.h>
#include <GitRemote.h>

#include <QMessageBox>
#include <QPushButton>

PullDlg::PullDlg(QSharedPointer<GitBase> git, const QString &text, QWidget *parent)
   : QDialog(parent)
   , ui(new Ui::PullDlg)
   , mGit(git)
{
   ui->setupUi(this);

   ui->lText->setText(text);
   ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Pull");

   setStyleSheet(GitQlientStyles::getStyles());
}

PullDlg::~PullDlg()
{
   delete ui;
}

void PullDlg::accept()
{
   QScopedPointer<GitRemote> git(new GitRemote(mGit));

   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   const auto ret = git->pull();
   QApplication::restoreOverrideCursor();

   if (ret.success)
   {
      emit signalRepositoryUpdated();

      QDialog::accept();
   }
   else
   {
      const auto errorMsg = ret.output.toString();

      if (errorMsg.toLower().contains("error: could not apply") && errorMsg.toLower().contains("causing a conflict"))
         emit signalPullConflict();
      else
         QMessageBox::critical(this, tr("Error while pulling"), errorMsg);
   }
}
