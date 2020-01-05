#include "TagDlg.h"
#include "ui_TagDlg.h"

#include <git.h>
#include <GitQlientStyles.h>

#include <QFile>

TagDlg::TagDlg(const QSharedPointer<Git> &git, const QString &sha, QWidget *parent)
   : QDialog(parent)
   , ui(new Ui::TagDlg)
   , mGit(git)
   , mSha(sha)
{
   setStyleSheet(GitQlientStyles::getStyles());

   ui->setupUi(this);

   connect(ui->leTagName, &QLineEdit::returnPressed, this, &TagDlg::accept);
   connect(ui->leTagMessage, &QLineEdit::returnPressed, this, &TagDlg::accept);

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
      auto ret = mGit->addTag(tagName, tagMessage, mSha);

      if (ret.success)
         QDialog::accept();
   }
}
