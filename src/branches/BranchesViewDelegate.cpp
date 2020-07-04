#include "BranchesViewDelegate.h"

#include <GitQlientStyles.h>
#include <GitQlientBranchItemRole.h>

#include <QPainter>

using namespace GitQlient;

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
      p->fillRect(newOpt.rect, GitQlientStyles::getGraphSelectionColor());

      if (i.column() == 0)
      {
         QRect rect(0, newOpt.rect.y(), newOpt.rect.x(), newOpt.rect.height());
         p->fillRect(rect, GitQlientStyles::getGraphSelectionColor());
      }
   }
   else if (newOpt.state & QStyle::State_MouseOver)
   {
      p->fillRect(newOpt.rect, GitQlientStyles::getGraphHoverColor());

      if (i.column() == 0)
      {
         QRect rect(0, newOpt.rect.y(), newOpt.rect.x(), newOpt.rect.height());
         p->fillRect(rect, GitQlientStyles::getGraphHoverColor());
      }
   }
   else
      p->fillRect(newOpt.rect, GitQlientStyles::getBackgroundColor());

   if (i.column() == 0)
   {
      if (i.data(IsLeaf).toBool())
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

   p->setPen(GitQlientStyles::getTextColor());

   QFontMetrics fm(newOpt.font);

   newOpt.font.setBold(i.data(Qt::UserRole).toBool());

   if (i.data().toString() == "detached")
      newOpt.font.setItalic(true);

   p->setFont(newOpt.font);

   const auto elidedText = fm.elidedText(i.data().toString(), Qt::ElideRight, newOpt.rect.width());

   p->drawText(newOpt.rect, elidedText, QTextOption(Qt::AlignLeft | Qt::AlignVCenter));
}

QSize BranchesViewDelegate::sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const
{
   return QSize(0, 25);
}
