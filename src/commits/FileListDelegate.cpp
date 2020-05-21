#include "FileListDelegate.h"

#include <GitQlientStyles.h>

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
      painter->fillRect(option.rect, GitQlientStyles::getGraphSelectionColor());
   else if (option.state & QStyle::State_MouseOver)
      painter->fillRect(option.rect, GitQlientStyles::getGraphHoverColor());

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
