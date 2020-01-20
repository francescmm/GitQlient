#include "FileWidget.h"

#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>

FileWidget::FileWidget(const QString &icon, const QString &text, QWidget *parent)
   : QFrame(parent)
   , mButton(new QPushButton(QIcon(icon), ""))
   , mText(new QLabel(text))
{
   const auto itemLayout = new QHBoxLayout(this);
   itemLayout->setContentsMargins(QMargins());
   const auto button = new QPushButton(QIcon(":/icons/add_tab"), "");
   mButton->setStyleSheet("max-width: 17px; min-width: 17px; max-height: 15px; min-height: 15px;");

   itemLayout->addWidget(mButton);
   itemLayout->addWidget(mText);

   connect(button, &QPushButton::clicked, this, &FileWidget::clicked);
}

QString FileWidget::text() const
{
   return mText->text();
}

void FileWidget::setText(const QString &text)
{
   mText->setText(text);
}
