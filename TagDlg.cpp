#include "TagDlg.h"
#include "ui_TagDlg.h"

#include <git.h>

TagDlg::TagDlg(const QString &sha, QWidget *parent)
   : QDialog(parent)
   , ui(new Ui::TagDlg)
   , mSha(sha)
{
   ui->setupUi(this);

   connect(ui->pbAccept, &QPushButton::clicked, this, &TagDlg::accept);
   connect(ui->pbCancel, &QPushButton::clicked, this, &QDialog::reject);
}

TagDlg::~TagDlg()
{
   delete ui;
}

void TagDlg::accept()
{
   auto tagName = ui->leTagName->text();
   auto tagMessage = ui->leTagMessage->text();

   if (!tagName.isEmpty() && !tagMessage.isEmpty())
   {
      tagName = tagName.trimmed();
      tagName = tagName.replace(" ", "_");

      tagMessage = tagMessage.trimmed();

      QByteArray output;
      auto ret = Git::getInstance()->addTag(tagName, tagMessage, mSha, output);

      if (ret)
      {
         QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
         ret = Git::getInstance()->pushTag(tagName, output);
         QApplication::restoreOverrideCursor();

         if (ret)
            QDialog::accept();
         else
            Git::getInstance()->removeTag(tagName, false, output);
      }
   }
}
