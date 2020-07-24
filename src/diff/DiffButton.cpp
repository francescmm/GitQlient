#include "DiffButton.h"

#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QIcon>
#include <QStyle>

DiffButton::DiffButton(const QString &text, const QString &icon, QWidget *parent)
   : DiffButton(text, icon, true, true, parent)
{
}

DiffButton::DiffButton(const QString &text, const QString &icon, bool selectable, bool closable, QWidget *parent)
   : QFrame(parent)
   , mSelectable(selectable)
   , mLabel(new QLabel())
   , mCloseBtn(new QPushButton())
{
   setAttribute(Qt::WA_DeleteOnClose);

   mCloseBtn->setIcon(QIcon(":/icons/close"));
   mCloseBtn->setObjectName("DiffButtonClose");

   mLabel->setText(text);
   mLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
   const auto layout = new QHBoxLayout(this);
   layout->setSpacing(10);
   layout->setContentsMargins(QMargins());
   layout->addSpacerItem(new QSpacerItem(10, 1, QSizePolicy::Fixed, QSizePolicy::Fixed));

   if (!icon.isEmpty())
   {
      const auto iconLabel = new QLabel();
      QIcon myIcon(icon);
      iconLabel->setPixmap(myIcon.pixmap(16, 16));
      layout->addWidget(iconLabel);
   }

   layout->addWidget(mLabel);

   if (closable)
      layout->addStretch();
   else
      layout->addSpacerItem(new QSpacerItem(10, 1, QSizePolicy::Fixed, QSizePolicy::Fixed));

   layout->addWidget(mCloseBtn);

   mCloseBtn->setVisible(closable);

   connect(mCloseBtn, &QPushButton::clicked, this, &DiffButton::close);
}

void DiffButton::setText(const QString &text) const
{
   mLabel->setText(text);
}

void DiffButton::setSelected()
{
   setProperty("pressed", true);
   style()->polish(this);
}

void DiffButton::mousePressEvent(QMouseEvent *e)
{
   mPressed = rect().contains(e->pos()) && e->button() == Qt::LeftButton;

   if (mPressed)
      emit clicked();

   if (mSelectable)
   {
      setProperty("pressed", mPressed);
      style()->polish(this);
   }
}

void DiffButton::setUnselected()
{
   mPressed = false;

   if (mSelectable)
   {
      setProperty("pressed", false);
      style()->polish(this);
   }
}

void DiffButton::setSelectable(bool isSelectable)
{
   mSelectable = isSelectable;
}

void DiffButton::setClosable(bool closable) const
{
   mCloseBtn->setVisible(closable);
}
