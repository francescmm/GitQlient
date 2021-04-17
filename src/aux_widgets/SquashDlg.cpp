#include "SquashDlg.h"
#include "ui_SquashDlg.h"

#include <GitCache.h>
#include <GitQlientSettings.h>

#include <CheckBox.h>
#include <QLabel>

SquashDlg::SquashDlg(QSharedPointer<GitCache> cache, const QStringList &shas, QWidget *parent)
   : QDialog(parent)
   , mCache(cache)
   , ui(new Ui::SquashDlg)
{
   ui->setupUi(this);

   setAttribute(Qt::WA_DeleteOnClose);

   mTitleMaxLength = GitQlientSettings().globalValue("commitTitleMaxLength", mTitleMaxLength).toInt();

   ui->lCounter->setText(QString::number(mTitleMaxLength));
   ui->leCommitTitle->setMaxLength(mTitleMaxLength);

   const auto commitsLayout = new QGridLayout();
   commitsLayout->setContentsMargins(10, 10, 10, 10);
   commitsLayout->setSpacing(10);
   commitsLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

   auto row = 0;
   for (const auto &sha : shas)
   {
      const auto check = new CheckBox("");
      mCheckboxList.append({ check, sha });
      commitsLayout->addWidget(check, row, 0);
      commitsLayout->addWidget(new QLabel(QString("<strong>(%1)</strong>").arg(sha.left(8))), row, 1);
      commitsLayout->addWidget(new QLabel(mCache->commitInfo(sha).shortLog()), row, 2);
      ++row;
   }

   commitsLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Fixed, QSizePolicy::Expanding), row, 0);

   ui->commitsFrame->setLayout(commitsLayout);
   ui->scrollArea->setWidgetResizable(true);

   connect(ui->leCommitTitle, &QLineEdit::textChanged, this, &SquashDlg::updateCounter);
   connect(ui->leCommitTitle, &QLineEdit::returnPressed, this, &SquashDlg::accept);
}

SquashDlg::~SquashDlg()
{
   delete ui;
}

void SquashDlg::updateCounter(const QString &text)
{
   ui->lCounter->setText(QString::number(mTitleMaxLength - text.count()));
}
