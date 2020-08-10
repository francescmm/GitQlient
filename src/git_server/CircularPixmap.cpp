#include "CircularPixmap.h"

#include <QPainter>
#include <QPainterPath>

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
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
   if (pixmap())
#endif
   {
      QPainter painter(this);
      painter.setRenderHint(QPainter::Antialiasing);

      QPainterPath path;
      path.addEllipse(0, 0, 50, 50);
      painter.setClipPath(path);
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
      painter.drawPixmap(0, 0, 50, 50, *pixmap());
#else
      painter.drawPixmap(0, 0, 50, 50, pixmap(Qt::ReturnByValue));
#endif
   }
}
