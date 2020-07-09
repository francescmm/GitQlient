#include "FileListDelegate.h"

#include <GitQlientStyles.h>

#include <QPainter>

constexpr auto Offset = 5;
constexpr auto DefaultHeight = 25.0;
constexpr auto HeightIncrement = 15.0;

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
   newOpt.rect.setX(newOpt.rect.x() + Offset);

   painter->drawText(newOpt.rect, index.data().toString(), QTextOption(Qt::AlignLeft | Qt::AlignVCenter));

   painter->restore();
}

QSize FileListDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
   QFontMetrics fm(option.font);
   const QAbstractItemModel *model = index.model();
   QString Text = model->data(index, Qt::DisplayRole).toString();
   QRect neededsize = fm.boundingRect(option.rect, Qt::TextWordWrap, Text);

   if (neededsize.height() < DefaultHeight)
      neededsize.setHeight(DefaultHeight);
   else
      neededsize.setHeight((neededsize.height() / DefaultHeight) * HeightIncrement + DefaultHeight);

   return QSize(option.rect.width(), neededsize.height());
}
