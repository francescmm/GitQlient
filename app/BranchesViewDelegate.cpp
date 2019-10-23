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

      if (i.column() == 0)
      {
         QRect rect(0, newOpt.rect.y(), newOpt.rect.x(), newOpt.rect.height());
         p->fillRect(rect, c);
      }
   }
   else if (newOpt.state & QStyle::State_MouseOver)
   {
      QColor c("#404142");
      c.setAlphaF(0.4);
      p->fillRect(newOpt.rect, c);

      if (i.column() == 0)
      {
         QRect rect(0, newOpt.rect.y(), newOpt.rect.x(), newOpt.rect.height());
         p->fillRect(rect, c);
      }
   }
   else
      p->fillRect(newOpt.rect, QColor("#2E2F30"));

   if (i.column() == 0)
   {
      if (i.data(Qt::UserRole + 2).toBool())
      {
         const auto width = newOpt.rect.x();
         QRect rectIcon(width - 20, newOpt.rect.y(), 20, newOpt.rect.height());
         QIcon icon(":/icons/repo_indicator");
         icon.paint(p, rectIcon);
      }
      else
      {
         const auto width = newOpt.rect.x();
         QRect rectIcon(width - 20, newOpt.rect.y(), 20, newOpt.rect.height());
         QIcon icon(":/icons/folder_indicator");
         icon.paint(p, rectIcon);
      }
   }

   p->setPen(QColor("white"));

   QFontMetrics fm(newOpt.font);

   newOpt.font.setBold(i.data(Qt::UserRole).toBool());
   p->setFont(newOpt.font);

   const auto elidedText = fm.elidedText(i.data().toString(), Qt::ElideRight, newOpt.rect.width());

   p->drawText(newOpt.rect, elidedText, QTextOption(Qt::AlignLeft | Qt::AlignVCenter));
}

QSize BranchesViewDelegate::sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const
{
   return QSize(0, 25);
}
