#include "FileListDelegate.h"

#include <QPainter>

const int FileListDelegate::OFFSET = 5;

FileListDelegate::FileListDelegate(QObject *parent)
   : QItemDelegate(parent)
{
}

void FileListDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
   painter->save();
   if (option.state & QStyle::State_Selected)
   {
      QColor c("#404142");
      c.setAlphaF(0.75);
      painter->fillRect(option.rect, c);
   }
   else if (option.state & QStyle::State_MouseOver)
   {
      QColor c("#404142");
      c.setAlphaF(0.4);
      painter->fillRect(option.rect, c);
   }

   painter->setPen(qvariant_cast<QColor>(index.data(Qt::ForegroundRole)));

   auto newOpt = option;
   newOpt.rect.setX(newOpt.rect.x() + OFFSET);

   QFontMetrics fm(newOpt.font);
   painter->drawText(newOpt.rect, fm.elidedText(index.data().toString(), Qt::ElideRight, newOpt.rect.width() - OFFSET),
                     QTextOption(Qt::AlignLeft | Qt::AlignVCenter));

   painter->restore();
}

QSize FileListDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &) const
{
   return QSize(option.rect.width(), 25);
}
