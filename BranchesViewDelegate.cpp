#include "BranchesViewDelegate.h"

#include <QPainter>

BranchesViewDelegate::BranchesViewDelegate(QObject *parent)
   : QStyledItemDelegate(parent)
{
}

void BranchesViewDelegate::paint(QPainter *p, const QStyleOptionViewItem &o, const QModelIndex &i) const
{
   p->setRenderHints(QPainter::Antialiasing);

   QStyleOptionViewItem newOpt(o);

   if (newOpt.state & QStyle::State_Selected)
   {
      QColor c("#404142");
      c.setAlphaF(0.75);
      p->fillRect(newOpt.rect, c);
   }
   else if (newOpt.state & QStyle::State_MouseOver)
   {
      QColor c("#404142");
      c.setAlphaF(0.4);
      p->fillRect(newOpt.rect, c);
   }
   else
      p->fillRect(newOpt.rect, QColor("#2E2F30"));

   QFontMetrics fm(newOpt.font);
   const auto textBoundingRect = fm.boundingRect(i.data().toString());
   const auto height = textBoundingRect.height();

   QRect textRect = newOpt.rect;
   textRect.setX(newOpt.rect.x() + 10);
   textRect.setY(newOpt.rect.y() + 25 - height - (25 - height) / 2);

   newOpt.font.setBold(i.data(Qt::UserRole).toBool());
   p->setFont(newOpt.font);
   p->drawText(textRect, i.data().toString());
}

QSize BranchesViewDelegate::sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const
{
   return QSize(0, 25);
}
