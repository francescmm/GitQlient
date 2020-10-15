#include "AddCodeReviewDialog.h"
#include "ui_AddCodeReviewDialog.h"

#include <QMessageBox>

AddCodeReviewDialog::AddCodeReviewDialog(ReviewMode mode, QWidget *parent)
   : QDialog(parent)
   , ui(new Ui::AddCodeReviewDialog)
   , mMode(mode)
{
   ui->setupUi(this);

   switch (mode)
   {
      case ReviewMode::Comment:
         setWindowTitle(tr("Add comment"));
         break;
      case ReviewMode::Approve:
         setWindowTitle(tr("Approve PR"));
         break;
      case ReviewMode::RequestChanges:
         setWindowTitle(tr("Request changes"));
      default:
         break;
   }

   setAttribute(Qt::WA_DeleteOnClose);
}

AddCodeReviewDialog::~AddCodeReviewDialog()
{
   delete ui;
}

void AddCodeReviewDialog::accept()
{
   if (const auto text = ui->teComment->toMarkdown(); !text.isEmpty())
      emit commentAdded(text);
   else if (mMode != ReviewMode::Approve)
      QMessageBox::warning(this, tr("Empty comment!"),
                           tr("The body cannot be empty when adding a comment or requesting changes."));
}
