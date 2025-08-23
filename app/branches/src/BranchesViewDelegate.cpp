#include "BranchesViewDelegate.h"

#include <GitQlientBranchItemRole.h>
#include <GitQlientStyles.h>

#include <QPainter>

using namespace GitQlient;

constexpr auto DefaultHeight = 30.0;

BranchesViewDelegate::BranchesViewDelegate(bool isTag, QObject *parent)
   : QStyledItemDelegate(parent)
   , mIsTag(isTag)
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

   static const auto iconSize = 20;
   static const auto offset = 5;

   if (i.column() == 0)
   {
      if (i.data(IsLeaf).toBool())
      {
         const auto width = newOpt.rect.x();
         QRect rectIcon(width - offset, newOpt.rect.y(), iconSize, newOpt.rect.height());
         QIcon icon(QString::fromUtf8(mIsTag ? ":/icons/tag_indicator" : ":/icons/repo_indicator"));
         icon.paint(p, rectIcon);
      }
      else
      {
         const auto width = newOpt.rect.x();
         QRect rectIcon(width - offset, newOpt.rect.y(), iconSize, newOpt.rect.height());
         QIcon icon(QString::fromUtf8(":/icons/folder_indicator"));
         icon.paint(p, rectIcon);
      }
   }

   p->setPen(GitQlientStyles::getTextColor());

   newOpt.font.setBold(i.data(Qt::UserRole).toBool());

   if (i.data().toString() == "detached")
      newOpt.font.setItalic(true);

   if (i.column() == 0)
      newOpt.rect.setX(newOpt.rect.x() + iconSize + offset);
   else
      newOpt.rect.setX(newOpt.rect.x() + iconSize - offset);

   p->setFont(newOpt.font);
   p->drawText(newOpt.rect, i.data().toString(), QTextOption(Qt::AlignLeft | Qt::AlignVCenter));
}

QSize BranchesViewDelegate::sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const
{
   return QSize(0, DefaultHeight);
}
