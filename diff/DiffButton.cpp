#include "DiffButton.h"

#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QIcon>
#include <QStyle>

DiffButton::DiffButton(const QString &text, const QString &icon, QWidget *parent)
   : QFrame(parent)
   , mCloseBtn(new QPushButton())
{
   setAttribute(Qt::WA_DeleteOnClose);

   mCloseBtn->setIcon(QIcon(":/icons/close"));
   mCloseBtn->setObjectName("DiffButtonClose");

   const auto label = new QLabel(text);
   label->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
   const auto layout = new QHBoxLayout();
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

   layout->addWidget(label);
   layout->addStretch();
   layout->addWidget(mCloseBtn);

   connect(mCloseBtn, &QPushButton::clicked, this, &DiffButton::close);

   setLayout(layout);
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

   setProperty("pressed", mPressed);
   style()->polish(this);
}

void DiffButton::setUnselected()
{
   mPressed = false;

   setProperty("pressed", false);
   style()->polish(this);
}
