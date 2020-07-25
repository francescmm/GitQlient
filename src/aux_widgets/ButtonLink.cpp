#include "ButtonLink.hpp"
#include <QStyle>
#include <QApplication>

ButtonLink::ButtonLink(QWidget *parent)
   : QLabel(parent)
{
   setContentsMargins(0, 0, 0, 0);
}

ButtonLink::ButtonLink(const QString &text, QWidget *parent)
   : QLabel(text, parent)
{
   setContentsMargins(0, 0, 0, 0);
}

void ButtonLink::mousePressEvent(QMouseEvent *e)
{
   Q_UNUSED(e);

   if (isEnabled())
      mPressed = true;
}

void ButtonLink::mouseReleaseEvent(QMouseEvent *event)
{
   Q_UNUSED(event);

   if (isEnabled() && mPressed)
   {
      emit clicked();
      // emit clicked (mCustomData);
   }
}

void ButtonLink::enterEvent(QEvent *event)
{
   Q_UNUSED(event);

   if (isEnabled())
   {
      QApplication::setOverrideCursor(Qt::PointingHandCursor);

      QFont f = font();
      f.setUnderline(true);
      setFont(f);
   }
}

void ButtonLink::leaveEvent(QEvent *event)
{
   Q_UNUSED(event);

   if (isEnabled())
   {
      QFont f = font();
      f.setUnderline(false);
      setFont(f);

      QApplication::restoreOverrideCursor();
   }
}
