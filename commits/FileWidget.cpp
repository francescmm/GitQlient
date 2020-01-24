#include "FileWidget.h"

#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>

FileWidget::FileWidget(const QIcon &icon, const QString &text, QWidget *parent)
   : QFrame(parent)
   , mIcon(icon)
   , mButton(new QPushButton(mIcon, ""))
   , mText(new QLabel(text))
{
   const auto itemLayout = new QHBoxLayout(this);
   itemLayout->setContentsMargins(QMargins());
   mButton->setStyleSheet("max-width: 17px; min-width: 17px; max-height: 15px; min-height: 15px;");

   itemLayout->addWidget(mButton);
   itemLayout->addWidget(mText);

   if (!icon.isNull())
      connect(mButton, &QPushButton::clicked, this, [this]() { emit clicked(); });
}

FileWidget::FileWidget(const QString &icon, const QString &text, QWidget *parent)
   : QFrame(parent)
   , mIcon(QIcon(icon))
   , mButton(new QPushButton(mIcon, ""))
   , mText(new QLabel(text))
{
   const auto itemLayout = new QHBoxLayout(this);
   itemLayout->setContentsMargins(QMargins());
   // const auto button = new QPushButton(QIcon(":/icons/add_tab"), "");
   mButton->setStyleSheet("max-width: 17px; min-width: 17px; max-height: 15px; min-height: 15px;");

   itemLayout->addWidget(mButton);
   itemLayout->addWidget(mText);

   if (!icon.isEmpty())
      connect(mButton, &QPushButton::clicked, this, [this]() { emit clicked(); });
}

QString FileWidget::text() const
{
   return mText->text();
}

void FileWidget::setText(const QString &text)
{
   mText->setText(text);
}
