#include "TagDlg.h"
#include "ui_TagDlg.h"

#include <git.h>

#include <QFile>

TagDlg::TagDlg(QSharedPointer<Git> git, const QString &sha, QWidget *parent)
   : QDialog(parent)
   , ui(new Ui::TagDlg)
   , mGit(git)
   , mSha(sha)
{
   QFile styles(":/stylesheet");

   if (styles.open(QIODevice::ReadOnly))
   {
      QFile colors(":/stylesheet_colors");
      QString colorsCss;

      if (colors.open(QIODevice::ReadOnly))
      {
         colorsCss = colors.readAll();
         colors.close();
      }

      setStyleSheet(styles.readAll() + colorsCss);
      styles.close();
   }

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
      auto ret = mGit->addTag(tagName, tagMessage, mSha, output);

      if (ret)
         QDialog::accept();
   }
}
