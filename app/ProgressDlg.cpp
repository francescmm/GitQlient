#include "ProgressDlg.h"

#include <GitQlientStyles.h>

#include <QKeyEvent>

ProgressDlg::ProgressDlg(const QString &labelText, const QString &cancelButtonText, int minimum, int maximum,
                         bool autoReset, bool autoClose)
   : QProgressDialog(labelText, cancelButtonText, minimum, maximum)
{
   setAutoClose(autoClose);
   setAutoReset(autoReset);
   setAttribute(Qt::WA_DeleteOnClose);
   setWindowModality(Qt::ApplicationModal);
   setWindowFlags(Qt::FramelessWindowHint);

   setStyleSheet(GitQlientStyles::getStyles());
}

void ProgressDlg::keyPressEvent(QKeyEvent *e)
{
   const auto key = e->key();

   if (key == Qt::Key_Escape)
      return;

   QDialog::keyPressEvent(e);
}

void ProgressDlg::closeEvent(QCloseEvent *e)
{
   if (!mPrepareToClose)
      e->ignore();
   else
      QDialog::closeEvent(e);
}

void ProgressDlg::close()
{
   mPrepareToClose = true;

   QDialog::close();
}
