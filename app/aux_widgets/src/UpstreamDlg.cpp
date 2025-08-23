#include "UpstreamDlg.h"
#include "ui_PullDlg.h"

#include <GitBase.h>
#include <GitBranches.h>
#include <GitQlientStyles.h>

#include <QMessageBox>
#include <QPushButton>

UpstreamDlg::UpstreamDlg(QSharedPointer<GitBase> git, const QString &text, QWidget *parent)
   : QDialog(parent)
   , ui(new Ui::PullDlg)
   , mGit(git)
{
   ui->setupUi(this);

   ui->lText->setText(text);
   ui->lQuestion->setText(tr("<strong>Would you like to reconfigure the upstream and push the branch?</strong>"));

   setStyleSheet(GitQlientStyles::getStyles());
}

UpstreamDlg::~UpstreamDlg()
{
   delete ui;
}

void UpstreamDlg::accept()
{
   QScopedPointer<GitBranches> git(new GitBranches(mGit));

   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   const auto ret = git->unsetUpstream();
   QApplication::restoreOverrideCursor();

   if (ret.success)
   {
      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
      const auto ret = git->pushUpstream(mGit->getCurrentBranch(), "origin", mGit->getCurrentBranch());
      QApplication::restoreOverrideCursor();

      QDialog::accept();
   }
   else
   {
      QMessageBox msgBox(QMessageBox::Critical, tr("Error while pulling"),
                         QString(tr("There were problems during the pull operation. Please, see the detailed "
                                    "description for more information.")),
                         QMessageBox::Ok, this);
      msgBox.setDetailedText(ret.output);
      msgBox.setStyleSheet(GitQlientStyles::getStyles());
      msgBox.exec();
   }
}
