#include "ClickableFrame.h"

#include <QMouseEvent>

ClickableFrame::ClickableFrame(QWidget *parent)
   : QFrame(parent)
{
}

void ClickableFrame::mousePressEvent(QMouseEvent *e)
{
   mPressed = rect().contains(e->pos()) && e->button() == Qt::LeftButton;
}

void ClickableFrame::mouseReleaseEvent(QMouseEvent *e)
{
   if (mPressed && rect().contains(e->pos()) && e->button() == Qt::LeftButton)
      emit clicked();
}
