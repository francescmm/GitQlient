#include "CircularPixmap.h"

#include <QPainter>

CircularPixmap::CircularPixmap(QWidget *parent)
   : QLabel(parent)
{
}

CircularPixmap::CircularPixmap(const QString &filePath, QWidget *parent)
   : QLabel(parent)
{
   QPixmap px(filePath);
   px = px.scaled(50, 50);

   setPixmap(filePath);
   setFixedSize(50, 50);
}

void CircularPixmap::paintEvent(QPaintEvent *)
{
   if (pixmap())
   {
      QPainter painter(this);
      painter.setRenderHint(QPainter::Antialiasing);

      QPainterPath path;
      path.addEllipse(0, 0, 50, 50);
      painter.setClipPath(path);
      painter.drawPixmap(0, 0, 50, 50, *pixmap());
   }
}
