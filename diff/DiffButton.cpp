#include "DiffButton.h"

#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QMouseEvent>

DiffButton::DiffButton(const QString &text, const QString &icon, QWidget *parent)
   : QFrame(parent)
   , mCloseBtn(new QPushButton())
{
   mCloseBtn->setIcon(QIcon(":/icons/close"));

   const auto label = new QLabel(text);
   const auto layout = new QHBoxLayout();
   layout->setSpacing(0);
   layout->setContentsMargins(QMargins());

   if (!icon.isEmpty())
      layout->addWidget(new QLabel());

   layout->addWidget(label);
   layout->addWidget(mCloseBtn);

   setLayout(layout);
}

void DiffButton::mousePressEvent(QMouseEvent *e)
{
   mPressed = rect().contains(e->pos()) && e->button() == Qt::LeftButton;
}

void DiffButton::mouseReleaseEvent(QMouseEvent *e)
{
   if (mPressed && rect().contains(e->pos()) && e->button() == Qt::LeftButton)
      emit clicked();
}
